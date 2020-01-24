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

#include <limits.h>
#include "Machine.hh"
#include "Report.hh"
#include "Debug.hh"
#include "Mutex.hh"
#include "DispatchQueue.hh"
#include "Network.hh"
#include "Graph.hh"
#include "Levelize.hh"
#include "Sdc.hh"
#include "SearchPred.hh"
#include "Bfs.hh"

#include "galois/Galois.h"
#include "galois/substrate/PerThreadStorage.h"

namespace sta {

bool use_galoised_bfs_iterator = true;

OriginalBfsIterator::OriginalBfsIterator(BfsIndex bfs_index,
			 Level level_min,
			 Level level_max,
			 SearchPred *search_pred,
			 StaState *sta) :
  StaState(sta),
  bfs_index_(bfs_index),
  level_min_(level_min),
  level_max_(level_max),
  search_pred_(search_pred)
{
  init();
}

void
OriginalBfsIterator::init()
{
  first_level_ = level_max_;
  last_level_ = level_min_;
  ensureSize();
}

void
OriginalBfsIterator::ensureSize()
{
  if (levelize_->levelized()) {
    unsigned max_level_1 = levelize_->maxLevel() + 1;
    if (queue_.size() < max_level_1)
      queue_.resize(max_level_1);
  }
}

OriginalBfsIterator::~OriginalBfsIterator()
{
}

void
OriginalBfsIterator::clear()
{
  Level level = first_level_;
  while (levelLessOrEqual(level, last_level_)) {
    VertexSeq &level_vertices = queue_[level];
    for (auto vertex : level_vertices) {
      if (vertex)
	vertex->setBfsInQueue(bfs_index_, false);
    }
    level_vertices.clear();
    incrLevel(level);
  }
  init();
}

void
OriginalBfsIterator::reportEntries(const Network *network)
{
  Level level = first_level_;
  while (levelLessOrEqual(level, last_level_)) {
    VertexSeq &level_vertices = queue_[level];
    if (!level_vertices.empty()) {
      printf("Level %d\n", level);
      for (auto vertex : level_vertices) {
	if (vertex)
	  printf(" %s\n", vertex->name(network));
      }
    }
    incrLevel(level);
  }
}

void 
OriginalBfsIterator::deleteEntries(Level level)
{
  VertexSeq &level_vertices = queue_[level];
  for (auto vertex : level_vertices) {
    if (vertex)
      vertex->setBfsInQueue(bfs_index_, false);
  }
  level_vertices.clear();
}

bool
OriginalBfsIterator::empty() const
{
  return levelLess(last_level_, first_level_);
}

void
OriginalBfsIterator::enqueueAdjacentVertices(Vertex *vertex)
{
  enqueueAdjacentVertices(vertex, search_pred_, level_max_);
}

void
OriginalBfsIterator::enqueueAdjacentVertices(Vertex *vertex,
				     SearchPred *search_pred)
{
  enqueueAdjacentVertices(vertex, search_pred, level_max_);
}

void
OriginalBfsIterator::enqueueAdjacentVertices(Vertex *vertex,
				     Level to_level)
{
  enqueueAdjacentVertices(vertex, search_pred_, to_level);
}

int
OriginalBfsIterator::visit(Level to_level,
		   VertexVisitor *visitor)
{
  int visit_count = 0;
  while (levelLessOrEqual(first_level_, last_level_)
	 && levelLessOrEqual(first_level_, to_level)) {
    VertexSeq &level_vertices = queue_[first_level_];
    incrLevel(first_level_);
    if (!level_vertices.empty()) {
      for (auto vertex : level_vertices) {
	if (vertex) {
	  vertex->setBfsInQueue(bfs_index_, false);
	  visitor->visit(vertex);
	  visit_count++;
	}
      }
      level_vertices.clear();
      visitor->levelFinished();
    }
  }
  return visit_count;
}

int
OriginalBfsIterator::visitParallel(Level to_level,
			   VertexVisitor *visitor)
{
  int visit_count = 0;
  if (!empty()) {
    if (thread_count_ <= 1)
      visit_count = visit(to_level, visitor);
    else {
      std::vector<VertexVisitor*> visitors;
      for (int i = 0; i < thread_count_; i++)
	visitors.push_back(visitor->copy());
      while (levelLessOrEqual(first_level_, last_level_)
	     && levelLessOrEqual(first_level_, to_level)) {
	VertexSeq &level_vertices = queue_[first_level_];
	incrLevel(first_level_);
	if (!level_vertices.empty()) {
	  for (auto vertex : level_vertices) {
	    if (vertex) {
	      vertex->setBfsInQueue(bfs_index_, false);
	      dispatch_queue_->dispatch( [vertex, &visitors](int i){ visitors[i]->visit(vertex); } );
	      visit_count++;
	    }
	  }
	  dispatch_queue_->finishTasks();
	  visitor->levelFinished();
	  level_vertices.clear();
	}
      }
    }
  }
  return visit_count;
}

bool
OriginalBfsIterator::hasNext()
{
  return hasNext(last_level_);
}

bool
OriginalBfsIterator::hasNext(Level to_level)
{
  findNext(to_level);
  return levelLessOrEqual(first_level_, last_level_)
     && !queue_[first_level_].empty();
}

Vertex *
OriginalBfsIterator::next()
{
  VertexSeq &level_vertices = queue_[first_level_];
  Vertex *vertex = level_vertices.back();
  level_vertices.pop_back();
  vertex->setBfsInQueue(bfs_index_, false);
  return vertex;
}

void
OriginalBfsIterator::findNext(Level to_level)
{
  while (levelLessOrEqual(first_level_, last_level_)
	 && levelLessOrEqual(first_level_, to_level)
	 && queue_[first_level_].empty())
    incrLevel(first_level_);
}

void
OriginalBfsIterator::enqueue(Vertex *vertex)
{
  debugPrint1(debug_, "bfs", 2, "enqueue %s\n", vertex->name(sdc_network_));
  if (!vertex->bfsInQueue(bfs_index_)) {
    Level level = vertex->level();
    UniqueLock lock(queue_lock_);
    if (!vertex->bfsInQueue(bfs_index_)) {
      vertex->setBfsInQueue(bfs_index_, true);
      queue_[level].push_back(vertex);

      if (levelLess(last_level_, level))
	last_level_ = level;
      if (levelLess(level, first_level_))
	first_level_ = level;
    }
  }
}

bool
OriginalBfsIterator::inQueue(Vertex *vertex)
{
  //  checkInQueue(vertex);
  return vertex->bfsInQueue(bfs_index_);
}

void
OriginalBfsIterator::checkInQueue(Vertex *vertex)
{
  Level level = vertex->level();
  if (static_cast<Level>(queue_.size()) > level) {
    for (auto v : queue_[level]) {
      if (v == vertex) {
	if (vertex->bfsInQueue(bfs_index_))
	  return;
	else
	  printf("extra %s\n", vertex->name(sdc_network_));
      }
    }
  }
  if (vertex->bfsInQueue(bfs_index_))
    printf("missing %s\n", vertex->name(sdc_network_));
}

void
OriginalBfsIterator::deleteVertexBefore(Vertex *vertex)
{
  remove(vertex);
}

// Remove by inserting null vertex pointer.
void
OriginalBfsIterator::remove(Vertex *vertex)
{
  // If the iterator has not been inited the queue will be empty.
  Level level = vertex->level();
  if (vertex->bfsInQueue(bfs_index_)
      && static_cast<Level>(queue_.size()) > level) {
    for (auto &v : queue_[level]) {
      if (v == vertex) {
	v = nullptr;
	vertex->setBfsInQueue(bfs_index_, false);
        break;
      }
    }
  }
}

////////////////////////////////////////////////////////////////

OriginalBfsFwdIterator::OriginalBfsFwdIterator(BfsIndex bfs_index,
			       SearchPred *search_pred,
			       StaState *sta) :
  OriginalBfsIterator(bfs_index, 0, INT_MAX, search_pred, sta)
{
}

// clear() without saving lists to list_free_.
OriginalBfsFwdIterator::~OriginalBfsFwdIterator()
{
  for (Level level = first_level_; level <= last_level_; level++)
    deleteEntries(level);
}

void
OriginalBfsFwdIterator::incrLevel(Level &level)
{
  level++;
}

bool
OriginalBfsFwdIterator::levelLessOrEqual(Level level1,
				 Level level2) const
{
  return level1 <= level2;
}

bool
OriginalBfsFwdIterator::levelLess(Level level1,
			  Level level2) const
{
  return level1 < level2;
}

void
OriginalBfsFwdIterator::enqueueAdjacentVertices(Vertex *vertex,
					SearchPred *search_pred,
					Level to_level)
{
  if (search_pred->searchFrom(vertex)) {
    VertexOutEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge *edge = edge_iter.next();
      Vertex *to_vertex = edge->to(graph_);
      if (to_vertex->level() <= to_level
	  && search_pred->searchThru(edge)
	  && search_pred->searchTo(to_vertex))
	enqueue(to_vertex);
    }
  }
}

////////////////////////////////////////////////////////////////

OriginalBfsBkwdIterator::OriginalBfsBkwdIterator(BfsIndex bfs_index,
				 SearchPred *search_pred,
				 StaState *sta) :
  OriginalBfsIterator(bfs_index, INT_MAX, 0, search_pred, sta)
{
}

// clear() without saving lists to list_free_.
OriginalBfsBkwdIterator::~OriginalBfsBkwdIterator()
{
  for (Level level = first_level_; level >= last_level_; level--)
    deleteEntries(level);
}

void
OriginalBfsBkwdIterator::incrLevel(Level &level)
{
  level--;
}

bool
OriginalBfsBkwdIterator::levelLessOrEqual(Level level1,
				  Level level2) const
{
  return level1 >= level2;
}

bool
OriginalBfsBkwdIterator::levelLess(Level level1,
			   Level level2) const
{
  return level1 > level2;
}

void
OriginalBfsBkwdIterator::enqueueAdjacentVertices(Vertex *vertex,
					 SearchPred *search_pred,
					 Level to_level)
{
  if (search_pred->searchTo(vertex)) {
    VertexInEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge *edge = edge_iter.next();
      Vertex *from_vertex = edge->from(graph_);
      if (from_vertex->level() >= to_level
	  && search_pred->searchFrom(from_vertex)
	  && search_pred->searchThru(edge))
	enqueue(from_vertex);
    }
  }
}

////////////////////////////////////////////////////////////////

GaloisedBfsIterator::GaloisedBfsIterator(BfsIndex bfs_index,
			 Level level_min,
			 Level level_max,
			 SearchPred *search_pred,
			 StaState *sta) :
  StaState(sta),
  bfs_index_(bfs_index),
  level_min_(level_min),
  level_max_(level_max),
  search_pred_(search_pred)
{
  init();
}

void
GaloisedBfsIterator::init()
{
  first_level_ = level_max_;
  last_level_ = level_min_;
  history_max_level_1_= 0;
  per_thread_min_level_.reset();
  per_thread_max_level_.reset();
  ensureSize();
}

void
GaloisedBfsIterator::ensureSize()
{
  if (levelize_->levelized()) {
    int max_level_1 = levelize_->maxLevel() + 1;
    if (queue_.size() < max_level_1)
      queue_.resize(max_level_1);
    // allocate worklist at corresponding levels
    if (history_max_level_1_ < max_level_1) {
      for (auto i = history_max_level_1_; i < max_level_1; i++) {
        queue_[i] = new galois::InsertBag<Vertex*>;
      }
    }
  }
}

GaloisedBfsIterator::~GaloisedBfsIterator()
{
}

void
GaloisedBfsIterator::clear()
{
  Level level = first_level_;
  while (levelLessOrEqual(level, last_level_)) {
    auto& level_vertices = *queue_[level];
    galois::do_all(
        galois::iterate(level_vertices),
        [&] (Vertex* vertex) {
          if (vertex)
	    vertex->setBfsInQueue(bfs_index_, false);
        }
    );
    level_vertices.clear();
    incrLevel(level);
  }
  init();
}

void
GaloisedBfsIterator::reportEntries(const Network *network)
{
  Level level = first_level_;
  while (levelLessOrEqual(level, last_level_)) {
    auto& level_vertices = *queue_[level];
    if (!level_vertices.empty()) {
      printf("Level %d\n", level);
      for (auto vertex : level_vertices) {
	if (vertex)
	  printf(" %s\n", vertex->name(network));
      }
    }
    incrLevel(level);
  }
}

void 
GaloisedBfsIterator::deleteEntries(Level level)
{
  auto& level_vertices = *queue_[level];
  galois::do_all(
      galois::iterate(level_vertices),
      [&] (Vertex* vertex) {
        if (vertex)
          vertex->setBfsInQueue(bfs_index_, false);
      }
  );
  level_vertices.clear();
}

bool
GaloisedBfsIterator::empty() const
{
  // need to call updateLevelBoundaries() before this
  return levelLess(last_level_, first_level_);
}

void
GaloisedBfsIterator::enqueueAdjacentVertices(Vertex *vertex)
{
  enqueueAdjacentVertices(vertex, search_pred_, level_max_);
}

void
GaloisedBfsIterator::enqueueAdjacentVertices(Vertex *vertex,
				     SearchPred *search_pred)
{
  enqueueAdjacentVertices(vertex, search_pred, level_max_);
}

void
GaloisedBfsIterator::enqueueAdjacentVertices(Vertex *vertex,
				     Level to_level)
{
  enqueueAdjacentVertices(vertex, search_pred_, to_level);
}

int
GaloisedBfsIterator::visit(Level to_level,
		   VertexVisitor *visitor)
{
  int visit_count = 0;
  updateLevelBoundaries();
  while (levelLessOrEqual(first_level_, last_level_)
	 && levelLessOrEqual(first_level_, to_level)) {
    auto& level_vertices = *queue_[first_level_];
    incrLevel(first_level_);
    if (!level_vertices.empty()) {
      for (auto vertex : level_vertices) {
	if (vertex) {
	  vertex->setBfsInQueue(bfs_index_, false);
	  visitor->visit(vertex);
	  visit_count++;
	}
      }
      level_vertices.clear();
      levelFinished(visitor);
    }
  }
  return visit_count;
}

int
GaloisedBfsIterator::visitParallel(Level to_level,
			   VertexVisitor *visitor)
{
  int visit_count = 0;
  updateLevelBoundaries();

  if (!empty()) {
    galois::setActiveThreads(threadCount());

    galois::substrate::PerThreadStorage<VertexVisitor*> visitors;
    for (int i = 0; i < galois::getActiveThreads(); i++)
      *(visitors.getRemote(i)) = visitor->copy();

    galois::GAccumulator<int> visit_count_local;
    visit_count_local.reset();

    while (levelLessOrEqual(first_level_, last_level_)
           && levelLessOrEqual(first_level_, to_level)) {
      auto& level_vertices = *queue_[first_level_];
      incrLevel(first_level_);
      if (!level_vertices.empty()) {
        galois::do_all(
            galois::iterate(level_vertices),
            [&] (Vertex* vertex) {
              if (vertex) {
                vertex->setBfsInQueue(bfs_index_, false);
                (*(visitors.getLocal()))->visit(vertex);
                visit_count_local += 1;
              }
            }
            , galois::steal()
            , galois::loopname("visitParallel")
	);
        levelFinished(visitor);
	level_vertices.clear();
      }
    }
    visit_count = visit_count_local.reduce();
  }
  return visit_count;
}

bool
GaloisedBfsIterator::hasNext()
{
  return hasNext(last_level_);
}

bool
GaloisedBfsIterator::hasNext(Level to_level)
{
  findNext(to_level);
  return levelLessOrEqual(first_level_, last_level_)
     && !queue_[first_level_]->empty();
}

Vertex *
GaloisedBfsIterator::next()
{
  auto& level_vertices = *queue_[first_level_];
  Vertex *vertex = *(level_vertices.begin());
  vertex->setBfsInQueue(bfs_index_, false);
  remove(vertex);

  galois::InsertBag<Vertex*> bag;
  galois::do_all(
      galois::iterate(level_vertices),
      [&] (Vertex* vertex) {
        if (vertex)
          bag.push_back(vertex);
      }
  );

  level_vertices.clear();
  galois::do_all(
      galois::iterate(bag),
      [&] (Vertex* vertex) {
        level_vertices.push(vertex);
      }
  );
  bag.clear();

  return vertex;
}

void
GaloisedBfsIterator::findNext(Level to_level)
{
  while (levelLessOrEqual(first_level_, last_level_)
	 && levelLessOrEqual(first_level_, to_level)
	 && queue_[first_level_]->empty())
    incrLevel(first_level_);
}

void
GaloisedBfsIterator::enqueue(Vertex *vertex)
{
  debugPrint1(debug_, "bfs", 2, "enqueue %s\n", vertex->name(sdc_network_));
  if (!vertex->bfsInQueue(bfs_index_)) {
    if (vertex->setBfsInQueue(bfs_index_, true)) {
      Level level = vertex->level();
      queue_[level]->push_back(vertex);
      per_thread_min_level_.update(level);
      per_thread_max_level_.update(level);
    }
  }
}

bool
GaloisedBfsIterator::inQueue(Vertex *vertex)
{
  //  checkInQueue(vertex);
  return vertex->bfsInQueue(bfs_index_);
}

void
GaloisedBfsIterator::checkInQueue(Vertex *vertex)
{
  Level level = vertex->level();
  if (static_cast<Level>(queue_.size()) > level) {
    for (auto v : *queue_[level]) {
      if (v == vertex) {
	if (vertex->bfsInQueue(bfs_index_))
	  return;
	else
	  printf("extra %s\n", vertex->name(sdc_network_));
      }
    }
  }
  if (vertex->bfsInQueue(bfs_index_))
    printf("missing %s\n", vertex->name(sdc_network_));
}

void
GaloisedBfsIterator::deleteVertexBefore(Vertex *vertex)
{
  remove(vertex);
}

// Remove by inserting null vertex pointer.
void
GaloisedBfsIterator::remove(Vertex *vertex)
{
  // If the iterator has not been inited the queue will be empty.
  Level level = vertex->level();
  if (vertex->bfsInQueue(bfs_index_)
      && static_cast<Level>(queue_.size()) > level) {
    for (auto &v : *queue_[level]) {
      if (v == vertex) {
	v = nullptr;
	vertex->setBfsInQueue(bfs_index_, false);
        break;
      }
    }
  }
}

void
GaloisedBfsIterator::levelFinished(VertexVisitor* visitor)
{
  if (visitor)
    visitor->levelFinished();
  updateLevelBoundaries();
}

////////////////////////////////////////////////////////////////

GaloisedBfsFwdIterator::GaloisedBfsFwdIterator(BfsIndex bfs_index,
			       SearchPred *search_pred,
			       StaState *sta) :
  GaloisedBfsIterator(bfs_index, 0, INT_MAX, search_pred, sta)
{
}

// clear() without saving lists to list_free_.
GaloisedBfsFwdIterator::~GaloisedBfsFwdIterator()
{
  for (Level level = first_level_; level <= last_level_; level++)
    deleteEntries(level);
}

void
GaloisedBfsFwdIterator::incrLevel(Level &level)
{
  level++;
}

bool
GaloisedBfsFwdIterator::levelLessOrEqual(Level level1,
				 Level level2) const
{
  return level1 <= level2;
}

bool
GaloisedBfsFwdIterator::levelLess(Level level1,
			  Level level2) const
{
  return level1 < level2;
}

void
GaloisedBfsFwdIterator::enqueueAdjacentVertices(Vertex *vertex,
					SearchPred *search_pred,
					Level to_level)
{
  if (search_pred->searchFrom(vertex)) {
    VertexOutEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge *edge = edge_iter.next();
      Vertex *to_vertex = edge->to(graph_);
      if (to_vertex->level() <= to_level
	  && search_pred->searchThru(edge)
	  && search_pred->searchTo(to_vertex))
	enqueue(to_vertex);
    }
  }
}

void
GaloisedBfsFwdIterator::updateLevelBoundaries() {
  auto minLv = per_thread_min_level_.reduce();
  if (levelLess(minLv, first_level_))
    first_level_ = minLv;
  per_thread_min_level_.reset();

  auto maxLv = per_thread_max_level_.reduce();
  if (levelLess(last_level_, maxLv))
    last_level_ = maxLv;
  per_thread_max_level_.reset();
}

////////////////////////////////////////////////////////////////

GaloisedBfsBkwdIterator::GaloisedBfsBkwdIterator(BfsIndex bfs_index,
				 SearchPred *search_pred,
				 StaState *sta) :
  GaloisedBfsIterator(bfs_index, INT_MAX, 0, search_pred, sta)
{
}

// clear() without saving lists to list_free_.
GaloisedBfsBkwdIterator::~GaloisedBfsBkwdIterator()
{
  for (Level level = first_level_; level >= last_level_; level--)
    deleteEntries(level);
}

void
GaloisedBfsBkwdIterator::incrLevel(Level &level)
{
  level--;
}

bool
GaloisedBfsBkwdIterator::levelLessOrEqual(Level level1,
				  Level level2) const
{
  return level1 >= level2;
}

bool
GaloisedBfsBkwdIterator::levelLess(Level level1,
			   Level level2) const
{
  return level1 > level2;
}

void
GaloisedBfsBkwdIterator::enqueueAdjacentVertices(Vertex *vertex,
					 SearchPred *search_pred,
					 Level to_level)
{
  if (search_pred->searchTo(vertex)) {
    VertexInEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge *edge = edge_iter.next();
      Vertex *from_vertex = edge->from(graph_);
      if (from_vertex->level() >= to_level
	  && search_pred->searchFrom(from_vertex)
	  && search_pred->searchThru(edge))
	enqueue(from_vertex);
    }
  }
}

void
GaloisedBfsBkwdIterator::updateLevelBoundaries() {
  auto maxLv = per_thread_max_level_.reduce();
  if (levelLess(maxLv, first_level_))
    first_level_ = maxLv;
  per_thread_max_level_.reset();

  auto minLv = per_thread_min_level_.reduce();
  if (levelLess(last_level_, minLv))
    last_level_ = minLv;
  per_thread_min_level_.reset();
}

////////////////////////////////////////////////////////////////

BfsIterator::BfsIterator(OriginalBfsIterator* original_iter,
                         GaloisedBfsIterator* galoised_iter) :
  original_iter_(original_iter),
  galoised_iter_(galoised_iter)
{
}

BfsIterator::~BfsIterator()
{
  delete galoised_iter_;
  delete original_iter_;
}

void
BfsIterator::ensureSize()
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->ensureSize();
  else
    original_iter_->ensureSize();
}

void
BfsIterator::clear()
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->clear();
  else
    original_iter_->clear();
}

bool
BfsIterator::empty() const
{
  if (use_galoised_bfs_iterator)
    return galoised_iter_->empty();
  else
    return original_iter_->empty();
}

void
BfsIterator::enqueue(Vertex *vertex)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->enqueue(vertex);
  else
    original_iter_->enqueue(vertex);
}

void
BfsIterator::enqueueAdjacentVertices(Vertex *vertex)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->enqueueAdjacentVertices(vertex);
  else
    original_iter_->enqueueAdjacentVertices(vertex);
}

void
BfsIterator::enqueueAdjacentVertices(Vertex *vertex,
			             SearchPred *search_pred)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->enqueueAdjacentVertices(vertex, search_pred);
  else
    original_iter_->enqueueAdjacentVertices(vertex, search_pred);
}

void
BfsIterator::enqueueAdjacentVertices(Vertex *vertex,
			             Level to_level)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->enqueueAdjacentVertices(vertex, to_level);
  else
    original_iter_->enqueueAdjacentVertices(vertex, to_level);
}

void
BfsIterator::enqueueAdjacentVertices(Vertex *vertex,
	 		             SearchPred *search_pred,
				     Level to_level)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->enqueueAdjacentVertices(vertex, search_pred, to_level);
  else
    original_iter_->enqueueAdjacentVertices(vertex, search_pred, to_level);
}

bool
BfsIterator::inQueue(Vertex *vertex)
{
  if (use_galoised_bfs_iterator)
    return galoised_iter_->inQueue(vertex);
  else
    return original_iter_->inQueue(vertex);
}

void
BfsIterator::checkInQueue(Vertex *vertex)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->checkInQueue(vertex);
  else
    original_iter_->checkInQueue(vertex);
}

void
BfsIterator::deleteVertexBefore(Vertex *vertex)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->deleteVertexBefore(vertex);
  else
    original_iter_->deleteVertexBefore(vertex);
}

void
BfsIterator::remove(Vertex *vertex)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->remove(vertex);
  else
    original_iter_->remove(vertex);
}

void
BfsIterator::reportEntries(const Network *network)
{
  if (use_galoised_bfs_iterator)
    galoised_iter_->reportEntries(network);
  else
    original_iter_->reportEntries(network);
}

bool
BfsIterator::hasNext()
{
  if (use_galoised_bfs_iterator)
    return galoised_iter_->hasNext();
  else
    return original_iter_->hasNext();
}

bool
BfsIterator::hasNext(Level to_level)
{
  if (use_galoised_bfs_iterator)
    return galoised_iter_->hasNext(to_level);
  else
    return original_iter_->hasNext(to_level);
}

Vertex *
BfsIterator::next()
{
  if (use_galoised_bfs_iterator)
    return galoised_iter_->next();
  else
    return original_iter_->next();
}

int
BfsIterator::visit(Level to_level,
	           VertexVisitor *visitor)
{
  if (use_galoised_bfs_iterator)
    return galoised_iter_->visit(to_level, visitor);
  else
    return original_iter_->visit(to_level, visitor);
}

int
BfsIterator::visitParallel(Level to_level,
		           VertexVisitor *visitor)
{
  if (use_galoised_bfs_iterator)
    return galoised_iter_->visitParallel(to_level, visitor);
  else
    return original_iter_->visitParallel(to_level, visitor);
}

void
BfsIterator::copyState(const StaState *sta)
{
  StaState::copyState(sta);
  // notify subcomponents
  galoised_iter_->copyState(sta);
  original_iter_->copyState(sta);
}

////////////////////////////////////////////////////////////////

BfsFwdIterator::BfsFwdIterator(BfsIndex bfs_index,
		               SearchPred *search_pred,
		               StaState *sta) :
  BfsIterator(
    new OriginalBfsFwdIterator(bfs_index, search_pred, sta),
    new GaloisedBfsFwdIterator(bfs_index, search_pred, sta))
{
}

BfsFwdIterator::~BfsFwdIterator()
{
}

////////////////////////////////////////////////////////////////

BfsBkwdIterator::BfsBkwdIterator(BfsIndex bfs_index,
		                 SearchPred *search_pred,
		                 StaState *sta) :
  BfsIterator(
    new OriginalBfsBkwdIterator(bfs_index, search_pred, sta),
    new GaloisedBfsBkwdIterator(bfs_index, search_pred, sta))
{
}

BfsBkwdIterator::~BfsBkwdIterator()
{
}

} // namespace
