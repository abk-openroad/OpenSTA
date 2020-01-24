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

#ifndef STA_GRAPH_H
#define STA_GRAPH_H

#include "Mutex.hh"
#include "DisallowCopyAssign.hh"
#include "Iterator.hh"
#include "Map.hh"
#include "Vector.hh"
#include "ObjectTable.hh"
#include "ArrayTable.hh"
#include "StaState.hh"
#include "LibertyClass.hh"
#include "NetworkClass.hh"
#include "Delay.hh"
#include "GraphClass.hh"
#include "VertexId.hh"
#include "PathVertexRep.hh"

namespace sta {

class MinMax;
class Sdc;
class PathVertexRep;

enum class LevelColor { white, gray, black };

typedef ArrayTable<Delay> DelayTable;
typedef ObjectTable<Vertex> VertexTable;
typedef ObjectTable<Edge> EdgeTable;
typedef ArrayTable<Arrival> ArrivalsTable;
typedef ArrayTable<PathVertexRep> PrevPathsTable;
typedef Map<const Pin*, Vertex*> PinVertexMap;
typedef Iterator<Edge*> VertexEdgeIterator;
typedef Map<const Pin*, float*> WidthCheckAnnotations;
typedef Map<const Pin*, float*> PeriodCheckAnnotations;
typedef Vector<DelayTable*> DelayTableSeq;
typedef ObjectId EdgeId;
typedef ObjectId ArrivalId;
typedef ObjectId PrevPathId;

static constexpr EdgeId edge_id_null = object_id_null;
static constexpr ObjectIdx edge_idx_null = object_id_null;
static constexpr ObjectIdx vertex_idx_null = object_id_null;
static constexpr ObjectIdx arrival_null = object_id_null;
static constexpr ObjectIdx prev_path_null = object_id_null;

// The graph acts as a BUILDER for the graph vertices and edges.
class Graph : public StaState
{
public:
  // slew_rf_count is
  //  0 no slews
  //  1 one slew for rise/fall
  //  2 rise/fall slews
  // ap_count is the dcalc analysis point count.
  Graph(StaState *sta,
	int slew_rf_count,
	bool have_arc_delays,
	DcalcAPIndex ap_count);
  void makeGraph();
  virtual ~Graph();

  // Number of arc delays and slews from sdf or delay calculation.
  virtual void setDelayCount(DcalcAPIndex ap_count);

  // Vertex functions.
  // Bidirect pins have two vertices.
  virtual Vertex *vertex(VertexId vertex_id) const;
  VertexId id(const Vertex *vertex) const;
  void makePinVertices(Pin *pin);
  void makePinVertices(Pin *pin,
		       Vertex *&vertex,
		       Vertex *&bidir_drvr_vertex);
  // Both vertices for bidirects.
  void pinVertices(const Pin *pin,
		   // Return values.
		   Vertex *&vertex,
		   Vertex *&bidirect_drvr_vertex) const;
  // Driver vertex for bidirects.
  Vertex *pinDrvrVertex(const Pin *pin) const;
  // Load vertex for bidirects.
  Vertex *pinLoadVertex(const Pin *pin) const;
  virtual void deleteVertex(Vertex *vertex);
  bool hasFaninOne(Vertex *vertex) const;
  VertexId vertexCount() { return vertices_->size(); }
  Arrival *makeArrivals(Vertex *vertex,
			uint32_t count);
  Arrival *arrivals(Vertex *vertex) const;
  void clearArrivals();
  PathVertexRep *makePrevPaths(Vertex *vertex,
			       uint32_t count);
  PathVertexRep *prevPaths(Vertex *vertex) const;
  void clearPrevPaths();
  // Slews are reported slews in seconds.
  // Reported slew are the same as those in the liberty tables.
  //  reported_slews = measured_slews / slew_derate_from_library
  // Measured slews are between slew_lower_threshold and slew_upper_threshold.
  virtual const Slew &slew(const Vertex *vertex,
			   const RiseFall *rf,
			   DcalcAPIndex ap_index);
  virtual void setSlew(Vertex *vertex,
		       const RiseFall *rf,
		       DcalcAPIndex ap_index,
		       const Slew &slew);

  // Edge functions.
  virtual Edge *edge(EdgeId edge_index) const;
  EdgeId id(const Edge *edge) const;
  virtual Edge *makeEdge(Vertex *from,
			 Vertex *to,
			 TimingArcSet *arc_set);
  virtual void makeWireEdge(Pin *from_pin,
			    Pin *to_pin);
  void makePinInstanceEdges(Pin *pin);
  void makeInstanceEdges(const Instance *inst);
  void makeWireEdgesToPin(Pin *to_pin);
  void makeWireEdgesThruPin(Pin *hpin);
  virtual void makeWireEdgesFromPin(Pin *drvr_pin);
  virtual void deleteEdge(Edge *edge);
  virtual ArcDelay arcDelay(const Edge *edge,
			    const TimingArc *arc,
			    DcalcAPIndex ap_index) const;
  virtual void setArcDelay(Edge *edge,
			   const TimingArc *arc,
			   DcalcAPIndex ap_index,
			   ArcDelay delay);
  // Alias for arcDelays using library wire arcs.
  virtual const ArcDelay &wireArcDelay(const Edge *edge,
				       const RiseFall *rf,
				       DcalcAPIndex ap_index);
  virtual void setWireArcDelay(Edge *edge,
			       const RiseFall *rf,
			       DcalcAPIndex ap_index,
			       const ArcDelay &delay);
  // Is timing arc delay annotated.
  bool arcDelayAnnotated(Edge *edge,
			 TimingArc *arc,
			 DcalcAPIndex ap_index) const;
  void setArcDelayAnnotated(Edge *edge,
			    TimingArc *arc,
			    DcalcAPIndex ap_index,
			    bool annotated);
  bool wireDelayAnnotated(Edge *edge,
			  const RiseFall *rf,
			  DcalcAPIndex ap_index) const;
  void setWireDelayAnnotated(Edge *edge,
			     const RiseFall *rf,
			     DcalcAPIndex ap_index,
			     bool annotated);
  // True if any edge arc is annotated.
  bool delayAnnotated(Edge *edge);
  int edgeCount() { return edges_->size(); }
  virtual int arcCount() { return arc_count_; }

  // Sdf width check annotation.
  void widthCheckAnnotation(const Pin *pin,
			    const RiseFall *rf,
			    DcalcAPIndex ap_index,
			    // Return values.
			    float &width,
			    bool &exists);
  void setWidthCheckAnnotation(const Pin *pin,
			       const RiseFall *rf,
			       DcalcAPIndex ap_index,
			       float width);

  // Sdf period check annotation.
  void periodCheckAnnotation(const Pin *pin,
			     DcalcAPIndex ap_index,
			    // Return values.
			    float &period,
			     bool &exists);
  void setPeriodCheckAnnotation(const Pin *pin,
				DcalcAPIndex ap_index,
				float period);
  // Remove all delay and slew annotations.
  void removeDelaySlewAnnotations();
  VertexSet *regClkVertices() { return &reg_clk_vertices_; }

protected:
  void makeVerticesAndEdges();
  Vertex *makeVertex(Pin *pin,
		     bool is_bidirect_drvr,
		     bool is_reg_clk);
  virtual void makeEdgeArcDelays(Edge *edge);
  void makePinVertices(const Instance *inst);
  void makeWireEdgesFromPin(Pin *drvr_pin,
			    PinSet &visited_drvrs);
  void makeWireEdges();
  virtual void makeInstDrvrWireEdges(Instance *inst,
				     PinSet &visited_drvrs);
  virtual void makePortInstanceEdges(const Instance *inst,
				     LibertyCell *cell,
                                     LibertyPort *from_to_port);
  void removeWidthCheckAnnotations();
  void removePeriodCheckAnnotations();
  void makeSlewTables(DcalcAPIndex count);
  void deleteSlewTables();
  void makeVertexSlews(Vertex *vertex);
  void makeArcDelayTables(DcalcAPIndex ap_count);
  void deleteArcDelayTables();
  void deleteInEdge(Vertex *vertex,
		    Edge *edge);
  void deleteOutEdge(Vertex *vertex,
		     Edge *edge);
  void removeDelays();
  void removeDelayAnnotated(Edge *edge);
  // User defined predicate to filter graph edges for liberty timing arcs.
  virtual bool filterEdge(TimingArcSet *) const { return true; }

  VertexTable *vertices_;
  EdgeTable *edges_;
  // Bidirect pins are split into two vertices:
  //  load/sink (top level output, instance pin input) vertex in pin_vertex_map
  //  driver/source (top level input, instance pin output) vertex
  //  in pin_bidirect_drvr_vertex_map
  PinVertexMap pin_bidirect_drvr_vertex_map_;
  int arc_count_;
  ArrivalsTable arrivals_;
  std::mutex arrivals_lock_;
  PrevPathsTable prev_paths_;
  std::mutex prev_paths_lock_;
  Vector<bool> arc_delay_annotated_;
  int slew_rf_count_;
  bool have_arc_delays_;
  DcalcAPIndex ap_count_;
  DelayTableSeq slew_tables_;	      // [ap_index][tr_index][vertex_id]
  VertexId slew_count_;
  DelayTableSeq arc_delays_;	      // [ap_index][edge_arc_index]
  // Sdf width check annotations.
  WidthCheckAnnotations *width_check_annotations_;
  // Sdf period check annotations.
  PeriodCheckAnnotations *period_check_annotations_;
  // Register/latch clock vertices to search from.
  VertexSet reg_clk_vertices_;

  friend class Vertex;
  friend class VertexIterator;
  friend class VertexInEdgeIterator;
  friend class VertexOutEdgeIterator;
  friend class MakeEdgesThruHierPin;

private:
  DISALLOW_COPY_AND_ASSIGN(Graph);
};

// Each Vertex corresponds to one network pin.
class Vertex
{
public:
  Vertex();
  Pin *pin() const { return pin_; }
  // Pin path with load/driver suffix for bidirects.
  const char *name(const Network *network) const;
  bool isBidirectDriver() const { return is_bidirect_drvr_; }
  bool isDriver(const Network *network) const;
  Level level() const { return level_; }
  void setLevel(Level level);
  bool isRoot() const{ return level_ == 0; }
  LevelColor color() const { return static_cast<LevelColor>(color_); }
  void setColor(LevelColor color);
  ArrivalId arrivals() const { return arrivals_; }
  void setArrivals(ArrivalId id);
  PrevPathId prevPaths() const { return prev_paths_; }
  void setPrevPaths(PrevPathId id);
  // Requireds optionally follow arrivals in the same array.
  bool hasRequireds() const { return has_requireds_; }
  void setHasRequireds(bool has_req);
  TagGroupIndex tagGroupIndex() const;
  void setTagGroupIndex(TagGroupIndex tag_index);
  // Slew is annotated by sdc set_annotated_transition cmd.
  bool slewAnnotated(const RiseFall *rf,
		     const MinMax *min_max) const;
  // True if any rise/fall analysis pt slew is annotated.
  bool slewAnnotated() const;
  void setSlewAnnotated(bool annotated,
			const RiseFall *rf,
			DcalcAPIndex ap_index);
  void removeSlewAnnotated();
  // Constant zero/one from simulation.
  bool isConstant() const;
  LogicValue simValue() const;
  void setSimValue(LogicValue value);
  bool isDisabledConstraint() const { return is_disabled_constraint_; }
  void setIsDisabledConstraint(bool disabled);
  // True when vertex has timing check edges that constrain it.
  bool hasChecks() const  { return has_checks_; }
  void setHasChecks(bool has_checks);
  bool isCheckClk() const { return is_check_clk_; }
  void setIsCheckClk(bool is_check_clk);
  bool isGatedClkEnable() const { return is_gated_clk_enable_; }
  void setIsGatedClkEnable(bool enable);
  bool hasDownstreamClkPin() const { return has_downstream_clk_pin_; }
  void setHasDownstreamClkPin(bool has_clk_pin);
  // Vertices are constrained if they have one or more of the
  // following timing constraints:
  //   output delay constraints
  //   data check constraints
  //   path delay constraints
  bool isConstrained() const { return is_constrained_; }
  void setIsConstrained(bool constrained);
  bool bfsInQueue(BfsIndex index) const;
  // Returns true if successful.
  bool setBfsInQueue(BfsIndex index, bool value);
  bool isRegClk() const { return is_reg_clk_; }
  bool crprPathPruningDisabled() const { return crpr_path_pruning_disabled_;}
  void setCrprPathPruningDisabled(bool disabled);
  bool requiredsPruned() const { return requireds_pruned_; }
  void setRequiredsPruned(bool pruned);
  
  // ObjectTable interface.
  ObjectIdx objectIdx() const { return object_idx_; }
  void setObjectIdx(ObjectIdx idx);

  static int transitionCount() { return 2; }  // rise/fall

protected:
  void init(Pin *pin,
	    bool is_bidirect_drvr,
	    bool is_reg_clk);

  Pin *pin_;
  ArrivalId arrivals_;
  PrevPathId prev_paths_;
  EdgeId in_edges_;		// Edges to this vertex.
  EdgeId out_edges_;		// Edges from this vertex.

  // 4 bytes
  unsigned int tag_group_index_:tag_group_index_bits; // 24
  // Each bit corresponds to a different BFS queue.
  unsigned int bfs_in_queue_:int(BfsIndex::bits); // 4
  unsigned int slew_annotated_:slew_annotated_bits;

  // for doing bfs_in_queue_ w/ compare and swap
  unsigned int bfs_in_queue_atomic_[BfsIndex::bits];

  // 4 bytes (32 bits)
  unsigned int level_:16;
  // Levelization search state.
  // LevelColor gcc barfs if this is dcl'd.
  unsigned color_:2;
  // LogicValue gcc barfs if this is dcl'd.
  unsigned sim_value_:3;
  bool has_requireds_:1;
  // Bidirect pins have two vertices.
  // This flag distinguishes the driver and load vertices.
  bool is_bidirect_drvr_:1;
  bool is_reg_clk_:1;
  bool is_disabled_constraint_:1;
  bool is_gated_clk_enable_:1;
  // Constrained by timing check edge.
  bool has_checks_:1;
  // Is the clock for a timing check.
  bool is_check_clk_:1;
  bool is_constrained_:1;
  bool has_downstream_clk_pin_:1;
  bool crpr_path_pruning_disabled_:1;
  bool requireds_pruned_:1;

  unsigned object_idx_:VertexTable::idx_bits;

private:
  DISALLOW_COPY_AND_ASSIGN(Vertex);

  friend class Graph;
  friend class Edge;
  friend class VertexInEdgeIterator;
  friend class VertexOutEdgeIterator;
};

// There is one Edge between each pair of pins that has a timing
// path between them.
class Edge
{
public:
  Edge();
  Vertex *to(const Graph *graph) const { return graph->vertex(to_); }
  Vertex *from(const Graph *graph) const { return graph->vertex(from_); }
  TimingRole *role() const;
  bool isWire() const;
  TimingSense sense() const;
  TimingArcSet *timingArcSet() const { return arc_set_; }
  void setTimingArcSet(TimingArcSet *set);
  ArcId arcDelays() const { return arc_delays_; }
  void setArcDelays(ArcId arc_delays);
  bool delayAnnotationIsIncremental() const;
  void setDelayAnnotationIsIncremental(bool is_incr);
  // Edge is disabled by set_disable_timing constraint.
  bool isDisabledConstraint() const;
  void setIsDisabledConstraint(bool disabled);
  // Timing sense for the to_pin function after simplifying the
  // function based constants on the instance pins.
  TimingSense simTimingSense() const;
  void setSimTimingSense(TimingSense sense);
  // Edge is disabled by constants in condition (when) function.
  bool isDisabledCond() const { return is_disabled_cond_; }
  void setIsDisabledCond(bool disabled);
  // Edge is disabled to break combinational loops.
  bool isDisabledLoop() const { return is_disabled_loop_; }
  void setIsDisabledLoop(bool disabled);
  // Edge is disabled to prevent converging clocks from merging (Xilinx).
  bool isBidirectInstPath() const { return is_bidirect_inst_path_; }
  void setIsBidirectInstPath(bool is_bidir);
  bool isBidirectNetPath() const { return is_bidirect_net_path_; }
  void setIsBidirectNetPath(bool is_bidir);

  // ObjectTable interface.
  ObjectIdx objectIdx() const { return object_idx_; }
  void setObjectIdx(ObjectIdx idx);

protected:
  void init(VertexId from,
	    VertexId to,
	    TimingArcSet *arc_set);

  TimingArcSet *arc_set_;
  VertexId from_;
  VertexId to_;
  EdgeId vertex_in_link_;		// Vertex in edges list.
  EdgeId vertex_out_next_;		// Vertex out edges doubly linked list.
  EdgeId vertex_out_prev_;
  ArcId arc_delays_;
  bool delay_annotation_is_incremental_:1;
  bool is_bidirect_inst_path_:1;
  bool is_bidirect_net_path_:1;
  // Timing sense from function and constants on edge instance.
  unsigned sim_timing_sense_:timing_sense_bit_count;
  bool is_disabled_constraint_:1;
  bool is_disabled_cond_:1;
  bool is_disabled_loop_:1;
  unsigned object_idx_:VertexTable::idx_bits;

private:
  DISALLOW_COPY_AND_ASSIGN(Edge);

  friend class Graph;
  friend class GraphDelays1;
  friend class GraphSlewsDelays1;
  friend class GraphSlewsDelays2;
  friend class Vertex;
  friend class VertexInEdgeIterator;
  friend class VertexOutEdgeIterator;
};

// Iterate over all graph vertices.
class VertexIterator : public Iterator<Vertex*>
{
public:
  explicit VertexIterator(Graph *graph);
  virtual bool hasNext() { return vertex_ || bidir_vertex_; }
  virtual Vertex *next();

private:
  DISALLOW_COPY_AND_ASSIGN(VertexIterator);
  bool findNextPin();
  void findNext();

  Graph *graph_;
  Network *network_;
  Instance *top_inst_;
  LeafInstanceIterator *inst_iter_;
  InstancePinIterator *pin_iter_;
  Vertex *vertex_;
  Vertex *bidir_vertex_;
};

class VertexInEdgeIterator : public VertexEdgeIterator
{
public:
  VertexInEdgeIterator(Vertex *vertex,
		       const Graph *graph);
  VertexInEdgeIterator(VertexId vertex_id,
		       const Graph *graph);
  bool hasNext() { return (next_ != nullptr); }
  Edge *next();

private:
  DISALLOW_COPY_AND_ASSIGN(VertexInEdgeIterator);

  Edge *next_;
  const Graph *graph_;
};

class VertexOutEdgeIterator : public VertexEdgeIterator
{
public:
  VertexOutEdgeIterator(Vertex *vertex,
			const Graph *graph);
  bool hasNext() { return (next_ != nullptr); }
  Edge *next();

private:
  DISALLOW_COPY_AND_ASSIGN(VertexOutEdgeIterator);

  Edge *next_;
  const Graph *graph_;
};

// Iterate over the edges through a hierarchical pin.
class EdgesThruHierPinIterator : public Iterator<Edge*>
{
public:
  EdgesThruHierPinIterator(const Pin *hpin,
			   Network *network,
			   Graph *graph);
  virtual bool hasNext() { return edge_iter_.hasNext(); }
  virtual Edge *next() { return edge_iter_.next(); }

private:
  DISALLOW_COPY_AND_ASSIGN(EdgesThruHierPinIterator);

  EdgeSet edges_;
  EdgeSet::Iterator edge_iter_;
};

} // namespace
#endif
