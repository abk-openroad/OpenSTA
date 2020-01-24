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

#include <limits>
#include "Machine.hh"
#include "Network.hh"
#include "DispatchQueue.hh"
#include "StaState.hh"

#include "galois/Galois.h"

namespace sta {

StaState::StaState() :
  report_(nullptr),
  debug_(nullptr),
  units_(nullptr),
  network_(nullptr),
  sdc_(nullptr),
  corners_(nullptr),
  graph_(nullptr),
  levelize_(nullptr),
  parasitics_(nullptr),
  arc_delay_calc_(nullptr),
  graph_delay_calc_(nullptr),
  sim_(nullptr),
  search_(nullptr),
  latches_(nullptr),
  thread_count_(1),
  dispatch_queue_(nullptr),
  pocv_enabled_(false),
  sigma_factor_(1.0)
{
  galois::setActiveThreads(thread_count_);
}

StaState::StaState(const StaState *sta) :
  report_(sta->report_),
  debug_(sta->debug_),
  units_(sta->units_),
  network_(sta->network_),
  sdc_network_(sta->sdc_network_),
  cmd_network_(sta->cmd_network_),
  sdc_(sta->sdc_),
  corners_(sta->corners_),
  graph_(sta->graph_),
  levelize_(sta->levelize_),
  parasitics_(sta->parasitics_),
  arc_delay_calc_(sta->arc_delay_calc_),
  graph_delay_calc_(sta->graph_delay_calc_),
  sim_(sta->sim_),
  search_(sta->search_),
  latches_(sta->latches_),
  thread_count_(sta->thread_count_),
  dispatch_queue_(sta->dispatch_queue_),
  pocv_enabled_(sta->pocv_enabled_),
  sigma_factor_(sta->sigma_factor_)
{
}

void
StaState::copyState(const StaState *sta)
{
  report_ = sta->report_;
  debug_ = sta->debug_;
  units_ = sta->units_;
  network_ = sta->network_;
  sdc_network_ = sta->sdc_network_;
  cmd_network_ = sta->cmd_network_;
  sdc_ = sta->sdc_;
  corners_ = sta->corners_;
  graph_ = sta->graph_;
  levelize_ = sta->levelize_;
  parasitics_ = sta->parasitics_;
  arc_delay_calc_ = sta->arc_delay_calc_;
  graph_delay_calc_ = sta->graph_delay_calc_;
  sim_ = sta->sim_;
  search_ = sta->search_;
  latches_ = sta->latches_;
  thread_count_ = sta->thread_count_;
  dispatch_queue_ = sta->dispatch_queue_;
  pocv_enabled_ = sta->pocv_enabled_;
  sigma_factor_ = sta->sigma_factor_;
}

NetworkEdit *
StaState::networkEdit()
{
  return dynamic_cast<NetworkEdit*>(network_);
}

NetworkEdit *
StaState::networkEdit() const
{
  return dynamic_cast<NetworkEdit*>(network_);
}

NetworkReader *
StaState::networkReader()
{
  return dynamic_cast<NetworkReader*>(network_);
}

NetworkReader *
StaState::networkReader() const
{
  return dynamic_cast<NetworkReader*>(network_);
}

void
StaState::setReport(Report *report)
{
  report_ = report;
}

void
StaState::setDebug(Debug *debug)
{
  debug_ = debug;
}

} // namespace
