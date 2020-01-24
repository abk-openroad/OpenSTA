// OpenSTA, Static Timing Analyzer
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <algorithm>
#include "Machine.hh"
#include "Report.hh"
#include "Debug.hh"
#include "Stats.hh"
#include "TimingRole.hh"
#include "PortDirection.hh"
#include "Network.hh"
#include "Graph.hh"
#include "GraphCmp.hh"
#include "SearchPred.hh"
#include "Sdc.hh"
#include "Levelize.hh"

namespace sta {

using std::max;

Levelize::Levelize(StaState *sta) :
  StaState(sta),
  search_pred_(new SearchPredNonLatch2(sta)),
  levelized_(false),
  levels_valid_(false),
  max_level_(0),
  level_space_(1),
  loops_(nullptr),
  observer_(nullptr)
{
}

Levelize::~Levelize()
{
  delete search_pred_;
  delete observer_;
  deleteLoops();
}

void
Levelize::setLevelSpace(Level space)
{
  level_space_ = space;
}

void
Levelize::setObserver(LevelizeObserver *observer)
{
  delete observer_;
  observer_ = observer;
}

void
Levelize::clear()
{
  levelized_ = false;
  levels_valid_ = false;
  roots_.clear();
  relevelize_from_.clear();
  clearLoopEdges();
  deleteLoops();
}

void
Levelize::clearLoopEdges()
{
  EdgeSet::Iterator edge_iter(disabled_loop_edges_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    edge->setIsDisabledLoop(false);
  }
  disabled_loop_edges_.clear();
}

void
Levelize::deleteLoops()
{
  if (loops_) {
    loops_->deleteContents();
    delete loops_;
    loops_ = nullptr;
    loop_edges_.clear();
  }
}

void
Levelize::ensureLevelized()
{
  if (!levels_valid_) {
    if (levelized_)
      relevelize();
    else
      levelize();
  }
}

// Depth first search.
// "Introduction to Algorithms", section 23.3 pg 478.
void
Levelize::levelize()
{
  Stats stats(debug_);
  debugPrint0(debug_, "levelize", 1, "levelize\n");
  max_level_ = 0;
  clearLoopEdges();
  deleteLoops();
  loops_ = new GraphLoopSeq;
  findRoots();
  VertexSeq roots;
  // Sort the roots so that loop breaking is stable in regressions.
  // In situations where port directions are broken pins may
  // be treated as bidirects, leading to a plethora of degenerate
  // roots that take forever to sort.
  if (roots.size() < 100)
    sortRoots(roots);
  levelizeFrom(roots);
  ensureLatchLevels();
  // Find vertices in cycles that are were not accessible from roots.
  levelizeCycles();
  levelized_ = true;
  levels_valid_ = true;
  stats.report("Levelize");
}

void
Levelize::findRoots()
{
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    Vertex *vertex = vertex_iter.next();
    setLevel(vertex, 0);
    if (isRoot(vertex)) {
      debugPrint1(debug_, "levelize", 2, "root %s\n", vertex->name(sdc_network_));
      roots_.insert(vertex);
      if (hasFanout(vertex, search_pred_, graph_))
	// Color roots with no fanout black so that they are
	// not treated as degenerate loops by levelizeCycles().
	vertex->setColor(LevelColor::black);
    }
    else
      vertex->setColor(LevelColor::white);
  }
}

// Root vertices have at no enabled (non-disabled) edges entering them
// and are not disabled.
bool
Levelize::isRoot(Vertex *vertex)
{
  if (search_pred_->searchTo(vertex)) {
    VertexInEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge *edge = edge_iter.next();
      Vertex *from_vertex = edge->from(graph_);
      if (search_pred_->searchFrom(from_vertex)
	  && search_pred_->searchThru(edge))
	return false;
    }
    // Bidirect pins are not treated as roots in this case.
    return !sdc_->bidirectDrvrSlewFromLoad(vertex->pin());
  }
  else
    return false;
}

void
Levelize::sortRoots(VertexSeq &roots)
{
  // roots_ is a set so insert/delete are fast for incremental changes.
  // Copy the set into a vector for sorting.
  VertexSet::Iterator root_iter(roots_);
  while (root_iter.hasNext()) {
    Vertex *root = root_iter.next();
    roots.push_back(root);
  }
  sort(roots, VertexNameLess(network_));
}

void
Levelize::levelizeFrom(VertexSeq &roots)
{
  VertexSeq::Iterator root_iter(roots);
  while (root_iter.hasNext()) {
    Vertex *root = root_iter.next();
    EdgeSeq path;
    visit(root, 0, level_space_, path);
  }
}

void
Levelize::visit(Vertex *vertex,
		Level level,
		Level level_space,
		EdgeSeq &path)
{
  Pin *from_pin = vertex->pin();
  debugPrint2(debug_, "levelize", 3, "level %d %s\n",
	      level, vertex->name(sdc_network_));
  vertex->setColor(LevelColor::gray);
  setLevel(vertex, level);
  max_level_ = max(level, max_level_);
  level += level_space;

  if (search_pred_->searchFrom(vertex)) {
    VertexOutEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge *edge = edge_iter.next();
      Vertex *to_vertex = edge->to(graph_);
      if (search_pred_->searchThru(edge)
	  && search_pred_->searchTo(to_vertex)) {
	LevelColor to_color = to_vertex->color();
	if (to_color == LevelColor::gray)
	  // Back edges form feedback loops.
	  recordLoop(edge, path);
	else if (to_color == LevelColor::white
		 || to_vertex->level() < level) {
	  path.push_back(edge);
	  visit(to_vertex, level, level_space, path);
	  path.pop_back();
	}
      }
      if (edge->role() == TimingRole::latchDtoQ())
	  latch_d_to_q_edges_.insert(edge);
    }
    // Levelize bidirect driver as if it was a fanout of the bidirect load.
    if (sdc_->bidirectDrvrSlewFromLoad(from_pin)
	&& !vertex->isBidirectDriver()) {
      Vertex *to_vertex = graph_->pinDrvrVertex(from_pin);
      if (search_pred_->searchTo(to_vertex)
	  && (to_vertex->color() == LevelColor::white
	      || to_vertex->level() < level))
	visit(to_vertex, level, level_space, path);
    }
  }
  vertex->setColor(LevelColor::black);
}

void
Levelize::recordLoop(Edge *edge,
		     EdgeSeq &path)
{
  debugPrint3(debug_, "levelize", 2, "Loop edge %s -> %s (%s)\n",
	      edge->from(graph_)->name(sdc_network_),
	      edge->to(graph_)->name(sdc_network_),
	      edge->role()->asString());
  // Do not record loops if they have been invalidated.
  if (loops_) {
    EdgeSeq *loop_edges = loopEdges(path, edge);
    GraphLoop *loop = new GraphLoop(loop_edges);
    loops_->push_back(loop);
    if (sdc_->dynamicLoopBreaking())
      sdc_->makeLoopExceptions(loop);
  }
  // Record disabled loop edges so they can be cleared without
  // traversing the entire graph to find them.
  disabled_loop_edges_.insert(edge);
  edge->setIsDisabledLoop(true);
}

EdgeSeq *
Levelize::loopEdges(EdgeSeq &path,
		    Edge *closing_edge)
{
  debugPrint0(debug_, "loop", 2, "Loop\n");
  EdgeSeq *loop_edges = new EdgeSeq;
  // Skip the "head" of the path up to where closing_edge closes the loop.
  Pin *loop_pin = closing_edge->to(graph_)->pin();
  bool copy = false;
  EdgeSeq::Iterator edge_iter(path);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    Pin *from_pin = edge->from(graph_)->pin();
    if (from_pin == loop_pin)
      copy = true;
    if (copy) {
      debugPrint2(debug_, "loop", 2, " %s -> %s\n",
		  edge->from(graph_)->name(sdc_network_),
		  edge->to(graph_)->name(sdc_network_));
      loop_edges->push_back(edge);
      loop_edges_.insert(edge);
    }
  }
  debugPrint2(debug_, "loop", 2, " %s -> %s\n",
	      closing_edge->from(graph_)->name(sdc_network_),
	      closing_edge->to(graph_)->name(sdc_network_));
  loop_edges->push_back(closing_edge);
  loop_edges_.insert(closing_edge);
  return loop_edges;
}

// Make sure latch D input level is not the same as the Q level.
// This is because the Q arrival depends on the D arrival and
// to find them in parallel they have to be scheduled separately
// to avoid a race condition.
void
Levelize::ensureLatchLevels()
{
  EdgeSet::Iterator latch_edge_iter(latch_d_to_q_edges_);
  while (latch_edge_iter.hasNext()) {
    Edge *edge = latch_edge_iter.next();
    Vertex *from = edge->from(graph_);
    Vertex *to = edge->to(graph_);
    if (from->level() == to->level())
      setLevel(from, from->level() + level_space_);
  }
  latch_d_to_q_edges_.clear();
}

void
Levelize::levelizeCycles()
{
  // Find vertices that were not discovered by searching from all
  // graph roots.
  VertexSeq uncolored;
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    Vertex *vertex = vertex_iter.next();
    if (vertex->color() == LevelColor::white
	&& search_pred_->searchFrom(vertex))
      uncolored.push_back(vertex);
  }

  // Sort cycle vertices so results are stable.
  sort(uncolored, VertexNameLess(network_));

  VertexSeq::Iterator uncolored_iter(uncolored);
  while (uncolored_iter.hasNext()) {
    Vertex *vertex = uncolored_iter.next();
    // Only search from and assign root status to vertices that
    // previous searches did not visit.  Otherwise "everybody is a
    // root".
    if (vertex->color() == LevelColor::white) {
      EdgeSeq path;
      roots_.insert(vertex);
      visit(vertex, 0, level_space_, path);
    }
  }
}

void
Levelize::invalid()
{
  debugPrint0(debug_, "levelize", 1, "levels invalid\n");
  clear();
}

void
Levelize::invalidFrom(Vertex *vertex)
{
  debugPrint1(debug_, "levelize", 1, "level invalid from %s\n",
	      vertex->name(sdc_network_));
  VertexInEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    Vertex *from_vertex = edge->from(graph_);
    relevelize_from_.insert(from_vertex);
  }
  relevelize_from_.insert(vertex);
  levels_valid_ = false;
}

void
Levelize::deleteVertexBefore(Vertex *vertex)
{
  roots_.erase(vertex);
  relevelize_from_.erase(vertex);
}

void
Levelize::relevelizeFrom(Vertex *vertex)
{
  debugPrint1(debug_, "levelize", 1, "invalid relevelize from %s\n",
	      vertex->name(sdc_network_));
  relevelize_from_.insert(vertex);
  levels_valid_ = false;
}

void
Levelize::deleteEdgeBefore(Edge *edge)
{
  if (loop_edges_.hasKey(edge)) {
    // Relevelize if a loop edge is removed.  Incremental levelization
    // fails because the DFS path will be missing.
    invalid();
    // Prevent refererence to deleted edge by clearLoopEdges().
    disabled_loop_edges_.erase(edge);
  }
}

// Incremental relevelization.
// Note that if vertices or edges are removed from the graph the
// downstream levels will NOT be reduced to the "correct" level (the
// search will immediately terminate without visiting downstream
// vertices because the new level is less than the existing level).
// This is acceptable because the BFS search that depends on the
// levels only requires that a vertex level be greater than that of
// its predecessors.
void
Levelize::relevelize()
{
  VertexSet::Iterator vertex_iter(relevelize_from_);
  while (vertex_iter.hasNext()) {
    Vertex *vertex = vertex_iter.next();
    debugPrint1(debug_, "levelize", 1, "relevelize from %s\n",
		vertex->name(sdc_network_));
    if (search_pred_->searchFrom(vertex)) {
      if (isRoot(vertex)) {
	setLevel(vertex, 0);
	roots_.insert(vertex);
      }
      EdgeSeq path;
      visit(vertex, vertex->level(), 1, path);
    }
  }
  ensureLatchLevels();
  levels_valid_ = true;
  relevelize_from_.clear();
}

bool
Levelize::isDisabledLoop(Edge *edge) const
{
  return disabled_loop_edges_.hasKey(edge);
}

void
Levelize::setLevel(Vertex  *vertex,
		   Level level)
{
  if (vertex->level() != level) {
    if (observer_)
      observer_->levelChangedBefore(vertex);
    vertex->setLevel(level);
  }
}

////////////////////////////////////////////////////////////////

GraphLoop::GraphLoop(EdgeSeq *edges) :
  edges_(edges)
{
}

GraphLoop::~GraphLoop()
{
  delete edges_;
}

bool
GraphLoop::isCombinational() const
{
  EdgeSeq::Iterator edge_iter(edges_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    TimingRole *role = edge->role();
    if (!(role == TimingRole::wire()
	  || role == TimingRole::combinational()
	  || role == TimingRole::tristateEnable()
	  || role == TimingRole::tristateDisable()))
      return false;
  }
  return true;
}

void
GraphLoop::report(Report *report,
		  Network *network,
		  Graph *graph) const
{
  bool first_edge = true;
  EdgeSeq::Iterator loop_edge_iter(edges_);
  while (loop_edge_iter.hasNext()) {
    Edge *edge = loop_edge_iter.next();
    if (first_edge)
      report->print(" %s\n", edge->from(graph)->name(network));
    report->print(" %s\n", edge->to(graph)->name(network));
    first_edge = false;
  }
}

} // namespace
