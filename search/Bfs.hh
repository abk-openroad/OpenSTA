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

#ifndef STA_BFS_H
#define STA_BFS_H

#include <mutex>
#include "DisallowCopyAssign.hh"
#include "Iterator.hh"
#include "Set.hh"
#include "StaState.hh"
#include "GraphClass.hh"
#include "VertexVisitor.hh"

#include "galois/Reduction.h"
#include "galois/Bag.h"

namespace sta {

class SearchPred;
class OriginalBfsFwdIterator;
class OriginalBfsBkwdIterator;
class GaloisedBfsFwdIterator;
class GaloisedBfsBkwdIterator;

extern bool use_galoised_bfs_iterator;

// OriginalLevelQueue is a vector of vertex vectors indexed by logic level.
typedef Vector<VertexSeq> OriginalLevelQueue;

// Abstract base class for Original forward and backward breadth first search iterators.
// Visit all of the vertices at a level before moving to the next.
// Use enqueue to seed the search.
// Use enqueueAdjacentVertices as a vertex is visited to queue the
// fanout vertices filtered by the search predicate.
//
// Vertices are marked as being in the queue by using a flag on
// the vertex indexed by bfs_index.  A unique flag is only needed
// if the BFS in in use when other BFS's are simultaneously in use.
class OriginalBfsIterator : public StaState, Iterator<Vertex*>
{
public:
  virtual ~OriginalBfsIterator();
  // Make sure that the BFS queue is deep enough for the max logic level.
  void ensureSize();
  // Reset to virgin state.
  void clear();
  bool empty() const;
  // Enqueue a vertex to search from.
  void enqueue(Vertex *vertex);
  // Enqueue vertices adjacent to a vertex.
  void enqueueAdjacentVertices(Vertex *vertex);
  void enqueueAdjacentVertices(Vertex *vertex,
			       SearchPred *search_pred);
  void enqueueAdjacentVertices(Vertex *vertex,
			       Level to_level);
  virtual void enqueueAdjacentVertices(Vertex *vertex,
				       SearchPred *search_pred,
				       Level to_level) = 0;
  bool inQueue(Vertex *vertex);
  void checkInQueue(Vertex *vertex);
  // Notify iterator that vertex will be deleted.
  void deleteVertexBefore(Vertex *vertex);
  void remove(Vertex *vertex);
  void reportEntries(const Network *network);

  virtual bool hasNext();
  bool hasNext(Level to_level);
  virtual Vertex *next();

  // Apply visitor to all vertices in the queue in level order.
  // Returns the number of vertices that are visited.
  virtual int visit(Level to_level,
		    VertexVisitor *visitor);
  // Apply visitor to all vertices in the queue in level order,
  // using threads to parallelize the visits. visitor must be thread safe.
  // Returns the number of vertices that are visited.
  int visitParallel(Level to_level,
		    VertexVisitor *visitor);

protected:
  OriginalBfsIterator(BfsIndex bfs_index,
	      Level level_min,
	      Level level_max,
	      SearchPred *search_pred,
	      StaState *sta);
  void init();
  void deleteEntries(Level level);
  virtual bool levelLess(Level level1,
			 Level level2) const = 0;
  virtual bool levelLessOrEqual(Level level1,
				Level level2) const = 0;
  virtual void incrLevel(Level &level) = 0;
  void findNext(Level to_level);
  void deleteEntries();

  BfsIndex bfs_index_;
  Level level_min_;
  Level level_max_;
  SearchPred *search_pred_;
  OriginalLevelQueue queue_;
  std::mutex queue_lock_;
  // Min (max) level of queued vertices.
  Level first_level_;
  // Max (min) level of queued vertices.
  Level last_level_;

  friend class OriginalBfsFwdIterator;
  friend class OriginalBfsBkwdIterator;

private:
  DISALLOW_COPY_AND_ASSIGN(OriginalBfsIterator);
};

class OriginalBfsFwdIterator : public OriginalBfsIterator
{
public:
  OriginalBfsFwdIterator(BfsIndex bfs_index,
		 SearchPred *search_pred,
		 StaState *sta);
  virtual ~OriginalBfsFwdIterator();
  virtual void enqueueAdjacentVertices(Vertex *vertex,
				       SearchPred *search_pred,
				       Level to_level);
  using OriginalBfsIterator::enqueueAdjacentVertices;

protected:
  virtual bool levelLessOrEqual(Level level1,
				Level level2) const;
  virtual bool levelLess(Level level1,
			 Level level2) const;
  virtual void incrLevel(Level &level);

private:
  DISALLOW_COPY_AND_ASSIGN(OriginalBfsFwdIterator);
};

class OriginalBfsBkwdIterator : public OriginalBfsIterator
{
public:
  OriginalBfsBkwdIterator(BfsIndex bfs_index,
		  SearchPred *search_pred,
		  StaState *sta);
  virtual ~OriginalBfsBkwdIterator();
  virtual void enqueueAdjacentVertices(Vertex *vertex,
				       SearchPred *search_pred,
				       Level to_level);
  using OriginalBfsIterator::enqueueAdjacentVertices;

protected:
  virtual bool levelLessOrEqual(Level level1,
				Level level2) const;
  virtual bool levelLess(Level level1,
			 Level level2) const;
  virtual void incrLevel(Level &level);

private:
  DISALLOW_COPY_AND_ASSIGN(OriginalBfsBkwdIterator);
};

// GaloisedLevelQueue is a vector of galois::InsertBag<Vertex*>* indexed by logic level.
typedef Vector<galois::InsertBag<Vertex*>*> GaloisedLevelQueue;

// Abstract base class for Galoised forward and backward breadth first search iterators.
// Visit all of the vertices at a level before moving to the next.
// Use enqueue to seed the search.
// Use enqueueAdjacentVertices as a vertex is visited to queue the
// fanout vertices filtered by the search predicate.
//
// Vertices are marked as being in the queue by using a flag on
// the vertex indexed by bfs_index.  A unique flag is only needed
// if the BFS in in use when other BFS's are simultaneously in use.
class GaloisedBfsIterator : public StaState, Iterator<Vertex*>
{
public:
  virtual ~GaloisedBfsIterator();
  // Make sure that the BFS queue is deep enough for the max logic level.
  void ensureSize();
  // Reset to virgin state.
  void clear();
  bool empty() const;
  // Enqueue a vertex to search from.
  void enqueue(Vertex *vertex);
  // Enqueue vertices adjacent to a vertex.
  void enqueueAdjacentVertices(Vertex *vertex);
  void enqueueAdjacentVertices(Vertex *vertex,
			       SearchPred *search_pred);
  void enqueueAdjacentVertices(Vertex *vertex,
			       Level to_level);
  virtual void enqueueAdjacentVertices(Vertex *vertex,
				       SearchPred *search_pred,
				       Level to_level) = 0;
  bool inQueue(Vertex *vertex);
  void checkInQueue(Vertex *vertex);
  // Notify iterator that vertex will be deleted.
  void deleteVertexBefore(Vertex *vertex);
  void remove(Vertex *vertex);
  void reportEntries(const Network *network);

  virtual bool hasNext();
  bool hasNext(Level to_level);
  virtual Vertex *next();

  // Apply visitor to all vertices in the queue in level order.
  // Returns the number of vertices that are visited.
  virtual int visit(Level to_level,
		    VertexVisitor *visitor);
  // Apply visitor to all vertices in the queue in level order,
  // using threads to parallelize the visits. visitor must be thread safe.
  // Returns the number of vertices that are visited.
  int visitParallel(Level to_level,
		    VertexVisitor *visitor);

protected:
  GaloisedBfsIterator(BfsIndex bfs_index,
	      Level level_min,
	      Level level_max,
	      SearchPred *search_pred,
	      StaState *sta);
  void init();
  void deleteEntries(Level level);
  virtual bool levelLess(Level level1,
			 Level level2) const = 0;
  virtual bool levelLessOrEqual(Level level1,
				Level level2) const = 0;
  virtual void incrLevel(Level &level) = 0;
  void findNext(Level to_level);
  void deleteEntries();
  void levelFinished(VertexVisitor* visitor);
  virtual void updateLevelBoundaries() = 0;

  BfsIndex bfs_index_;
  Level level_min_;
  Level level_max_;
  SearchPred *search_pred_;
  GaloisedLevelQueue queue_;
  // Min (max) level of queued vertices.
  Level first_level_;
  // Max (min) level of queued vertices.
  Level last_level_;
  // Historical max level of queued vertices.
  Level history_max_level_1_;
  galois::GReduceMax<Level> per_thread_max_level_;
  galois::GReduceMin<Level> per_thread_min_level_;

  friend class GaloisedBfsFwdIterator;
  friend class GaloisedBfsBkwdIterator;

private:
  DISALLOW_COPY_AND_ASSIGN(GaloisedBfsIterator);
};

class GaloisedBfsFwdIterator : public GaloisedBfsIterator
{
public:
  GaloisedBfsFwdIterator(BfsIndex bfs_index,
		 SearchPred *search_pred,
		 StaState *sta);
  virtual ~GaloisedBfsFwdIterator();
  virtual void enqueueAdjacentVertices(Vertex *vertex,
				       SearchPred *search_pred,
				       Level to_level);
  using GaloisedBfsIterator::enqueueAdjacentVertices;

protected:
  virtual bool levelLessOrEqual(Level level1,
				Level level2) const;
  virtual bool levelLess(Level level1,
			 Level level2) const;
  virtual void incrLevel(Level &level);
  virtual void updateLevelBoundaries();

private:
  DISALLOW_COPY_AND_ASSIGN(GaloisedBfsFwdIterator);
};

class GaloisedBfsBkwdIterator : public GaloisedBfsIterator
{
public:
  GaloisedBfsBkwdIterator(BfsIndex bfs_index,
		  SearchPred *search_pred,
		  StaState *sta);
  virtual ~GaloisedBfsBkwdIterator();
  virtual void enqueueAdjacentVertices(Vertex *vertex,
				       SearchPred *search_pred,
				       Level to_level);
  using GaloisedBfsIterator::enqueueAdjacentVertices;

protected:
  virtual bool levelLessOrEqual(Level level1,
				Level level2) const;
  virtual bool levelLess(Level level1,
			 Level level2) const;
  virtual void incrLevel(Level &level);
  virtual void updateLevelBoundaries();

private:
  DISALLOW_COPY_AND_ASSIGN(GaloisedBfsBkwdIterator);
};

class BfsIterator: public StaState, Iterator<Vertex*>
{
public:
  virtual ~BfsIterator();
  // Make sure that the BFS queue is deep enough for the max logic level.
  virtual void ensureSize();
  // Reset to virgin state.
  virtual void clear();
  virtual bool empty() const;
  // Enqueue a vertex to search from.
  virtual void enqueue(Vertex *vertex);
  // Enqueue vertices adjacent to a vertex.
  virtual void enqueueAdjacentVertices(Vertex *vertex);
  virtual void enqueueAdjacentVertices(Vertex *vertex,
			       SearchPred *search_pred);
  virtual void enqueueAdjacentVertices(Vertex *vertex,
			       Level to_level);
  virtual void enqueueAdjacentVertices(Vertex *vertex,
				       SearchPred *search_pred,
				       Level to_level);
  virtual bool inQueue(Vertex *vertex);
  virtual void checkInQueue(Vertex *vertex);
  // Notify iterator that vertex will be deleted.
  virtual void deleteVertexBefore(Vertex *vertex);
  virtual void remove(Vertex *vertex);
  virtual void reportEntries(const Network *network);

  virtual bool hasNext();
  virtual bool hasNext(Level to_level);
  virtual Vertex *next();

  // Apply visitor to all vertices in the queue in level order.
  // Returns the number of vertices that are visited.
  virtual int visit(Level to_level,
		    VertexVisitor *visitor);
  // Apply visitor to all vertices in the queue in level order,
  // using threads to parallelize the visits. visitor must be thread safe.
  // Returns the number of vertices that are visited.
  virtual int visitParallel(Level to_level,
		    VertexVisitor *visitor);

  virtual void copyState(const StaState *sta);

protected:
  BfsIterator(OriginalBfsIterator* original_iter,
              GaloisedBfsIterator* galoised_iter);

  OriginalBfsIterator* original_iter_;
  GaloisedBfsIterator* galoised_iter_;

private:
  DISALLOW_COPY_AND_ASSIGN(BfsIterator); 
};

class BfsFwdIterator: public BfsIterator
{
public:
  BfsFwdIterator(BfsIndex bfs_index,
		 SearchPred *search_pred,
		 StaState *sta);
  virtual ~BfsFwdIterator();

private:
  DISALLOW_COPY_AND_ASSIGN(BfsFwdIterator);
};

class BfsBkwdIterator: public BfsIterator
{
public:
  BfsBkwdIterator(BfsIndex bfs_index,
		 SearchPred *search_pred,
		 StaState *sta);
  virtual ~BfsBkwdIterator();

private:
  DISALLOW_COPY_AND_ASSIGN(BfsBkwdIterator);
};

} // namespace
#endif
