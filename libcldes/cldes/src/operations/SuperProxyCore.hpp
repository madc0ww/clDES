/*
 =========================================================================
 This file is part of clDES

 clDES: an OpenCL library for Discrete Event Systems computing.

 clDES is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 clDES is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with clDES.  If not, see <http://www.gnu.org/licenses/>.

 Copyright (c) 2018 - Adriano Mourao <adrianomourao@protonmail.com>
                      madc0ww @ [https://github.com/madc0ww]

 LacSED - Laboratório de Sistemas a Eventos Discretos
 Universidade Federal de Minas Gerais

 File: cldes/src/operations/SuperProxyCore.hpp
 Description: SuperProxy methods definitions
 =========================================================================
*/
/*!
 * \file cldes/src/operations/SuperProxyCore.hpp
 *
 * \author Adriano Mourao \@madc0ww
 * \date 2018-06-16
 *
 * Virtual Proxy for the monolithic supervisor synthesis.
 */

namespace cldes {
template<uint8_t NEvents, typename StorageIndex>
op::SuperProxy<NEvents, StorageIndex>::SuperProxy(
  DESystemBase const& aSys0,
  DESystemBase const& aSys1,
  EventsTableHost const& aNonContr)
  : DESystemBase{ aSys0.GetStatesNumber() * aSys1.GetStatesNumber(),
                  aSys1.GetInitialState() * aSys0.GetStatesNumber() +
                    aSys0.GetInitialState() }
  , sys0_{ aSys0 }
  , sys1_{ aSys1 }
{
    sys_ptr_ = nullptr;
    n_states_sys0_ = aSys0.GetStatesNumber();

    auto const in_both = aSys0.GetEvents() & aSys1.GetEvents();

    only_in_0_ = aSys0.GetEvents() ^ in_both;
    only_in_1_ = aSys1.GetEvents() ^ in_both;

    // Initialize events_ from base class
    this->events_ = aSys0.GetEvents() | aSys1.GetEvents();

    for (auto q0 : aSys0.GetMarkedStates()) {
        for (auto q1 : aSys1.GetMarkedStates()) {
            this->marked_states_.emplace(q1 * n_states_sys0_ + q0);
        }
    }
}

template<uint8_t NEvents, typename StorageIndex>
op::SuperProxy<NEvents, StorageIndex>::SuperProxy(
  DESVector<NEvents, StorageIndex> const& aPlants,
  DESVector<NEvents, StorageIndex> const& aSpecs,
  EventsTableHost const& aNonContr)
{
}

template<uint8_t NEvents, typename StorageIndex>
op::SuperProxy<NEvents, StorageIndex>::operator DESystem()
{
    std::sort(virtual_states_.begin(), virtual_states_.end());
    SynchronizeStage2(*this);

    // Allocate memory for the real sys
    sys_ptr_ = std::make_shared<DESystem>(DESystem{});

    sys_ptr_->states_number_ = std::move(this->states_number_);
    sys_ptr_->init_state_ = std::move(this->init_state_);
    sys_ptr_->marked_states_ = std::move(this->marked_states_);
    sys_ptr_->states_events_ = std::move(this->states_events_);
    sys_ptr_->inv_states_events_ = std::move(this->inv_states_events_);
    sys_ptr_->events_ = std::move(this->events_);

    // Resize adj matrices
    sys_ptr_->graph_.resize(this->states_number_, this->states_number_);
    sys_ptr_->bit_graph_.resize(this->states_number_, this->states_number_);

    // Move triplets to graph storage
    sys_ptr_->graph_.setFromTriplets(triplet_.begin(), triplet_.end());
    sys_ptr_->bit_graph_.setFromTriplets(
      bittriplet_.begin(), bittriplet_.end(), [](bool const&, bool const&) {
          return true;
      });

    triplet_.clear();
    bittriplet_.clear();

    sys_ptr_->graph_.makeCompressed();
    sys_ptr_->bit_graph_.makeCompressed();

    return *sys_ptr_;
}

template<uint8_t NEvents, typename StorageIndex>
op::SuperProxy<NEvents, StorageIndex>::operator DESystem() const
{
    auto cp = *this;
    return DESystem(cp);
}

template<uint8_t NEvents, typename StorageIndex>
bool
op::SuperProxy<NEvents, StorageIndex>::IsVirtual() const
{
    return true;
}

template<uint8_t NEvents, typename StorageIndex>
std::shared_ptr<DESystemBase<NEvents, StorageIndex>>
op::SuperProxy<NEvents, StorageIndex>::Clone() const
{
    std::shared_ptr<DESystemBase> this_ptr =
      std::make_shared<SuperProxy>(*this);
    return this_ptr;
}

template<uint8_t NEvents, typename StorageIndex>
bool
op::SuperProxy<NEvents, StorageIndex>::ContainsTrans(
  StorageIndex const& aQ,
  ScalarType const& aEvent) const
{
    if (!this->events_.test(aEvent)) {
        return false;
    }
    // q = (qx, qy)
    auto const qx = aQ % n_states_sys0_;
    auto const qy = aQ / n_states_sys0_;

    auto const in_x = sys0_.ContainsTrans(qx, aEvent);
    auto const in_y = sys1_.ContainsTrans(qy, aEvent);

    auto contains = false;

    if ((in_x && in_y) || (in_x && only_in_0_.test(aEvent)) ||
        (in_y && only_in_1_.test(aEvent))) {
        contains = true;
    }

    return contains;
}

template<uint8_t NEvents, typename StorageIndex>
typename op::SuperProxy<NEvents, StorageIndex>::StorageIndexSigned
op::SuperProxy<NEvents, StorageIndex>::Trans(StorageIndex const& aQ,
                                             ScalarType const& aEvent) const
{
    if (!this->events_.test(aEvent)) {
        return -1;
    }
    // q = (qx, qy)
    auto const qx = aQ % n_states_sys0_;
    auto const qy = aQ / n_states_sys0_;

    auto const in_x = sys0_.ContainsTrans(qx, aEvent);
    auto const in_y = sys1_.ContainsTrans(qy, aEvent);

    if (!((in_x && in_y) || (in_x && only_in_0_.test(aEvent)) ||
          (in_y && only_in_1_.test(aEvent)))) {
        return -1;
    }

    if (in_x && in_y) {
        auto const q0 = sys0_.Trans(qx, aEvent);
        auto const q1 = sys1_.Trans(qy, aEvent);

        return q1 * n_states_sys0_ + q0;
    } else if (in_x) {
        auto const q0 = sys0_.Trans(qx, aEvent);
        return qy * n_states_sys0_ + q0;
    } else { // in_y
        auto const q1 = sys1_.Trans(qy, aEvent);
        return q1 * n_states_sys0_ + qx;
    }
}

template<uint8_t NEvents, typename StorageIndex>
bool
op::SuperProxy<NEvents, StorageIndex>::ContainsInvTrans(
  StorageIndex const& aQ,
  ScalarType const& aEvent) const
{
    if (!this->events_.test(aEvent)) {
        return false;
    }
    // q = (qx, qy)
    auto const qx = aQ % n_states_sys0_;
    auto const qy = aQ / n_states_sys0_;

    auto const in_x = sys0_.ContainsInvTrans(qx, aEvent);
    auto const in_y = sys1_.ContainsInvTrans(qy, aEvent);

    auto contains = false;

    if ((in_x && in_y) || (in_x && only_in_0_.test(aEvent)) ||
        (in_y && only_in_1_.test(aEvent))) {
        contains = true;
    }

    return contains;
}

template<uint8_t NEvents, typename StorageIndex>
StatesArray<StorageIndex>
op::SuperProxy<NEvents, StorageIndex>::InvTrans(StorageIndex const& aQ,
                                                ScalarType const& aEvent) const
{
    StatesArray<StorageIndex> inv_transitions;

    if (!this->events_.test(aEvent)) {
        return inv_transitions;
    }
    // q = (qx, qy)
    auto const qx = aQ % n_states_sys0_;
    auto const qy = aQ / n_states_sys0_;

    auto const in_x = sys0_.ContainsInvTrans(qx, aEvent);
    auto const in_y = sys1_.ContainsInvTrans(qy, aEvent);

    if (!((in_x && in_y) || (in_x && only_in_0_.test(aEvent)) ||
          (in_y && only_in_1_.test(aEvent)))) {
        return inv_transitions;
    }

    if (in_x && in_y) {
        auto const inv_trans_0 = sys0_.InvTrans(qx, aEvent);
        auto const inv_trans_1 = sys1_.InvTrans(qy, aEvent);

        inv_transitions.reserve(inv_trans_0.size() + inv_trans_1.size());

        for (auto q0 : inv_trans_0) {
            for (auto q1 : inv_trans_1) {
                auto const q_from = q1 * n_states_sys0_ + q0;
                inv_transitions.push_back(q_from);
            }
        }
    } else if (in_x) {
        auto const inv_trans_0 = sys0_.InvTrans(qx, aEvent);

        inv_transitions.reserve(inv_trans_0.size());

        for (auto q : inv_trans_0) {
            auto const q_from = qy * n_states_sys0_ + q;
            inv_transitions.push_back(q_from);
        }
    } else { // in_y
        auto const inv_trans_1 = sys1_.InvTrans(qy, aEvent);

        inv_transitions.reserve(inv_trans_1.size());

        for (auto q : inv_trans_1) {
            auto const q_from = q * n_states_sys0_ + qx;
            inv_transitions.push_back(q_from);
        }
    }

    return inv_transitions;
}

template<uint8_t NEvents, typename StorageIndex>
EventsSet<NEvents>
op::SuperProxy<NEvents, StorageIndex>::GetStateEvents(
  StorageIndex const& aQ) const
{
    auto const state_event_0 = sys0_.GetStateEvents(aQ % n_states_sys0_);
    auto const state_event_1 = sys1_.GetStateEvents(aQ / n_states_sys0_);
    auto const state_event = (state_event_0 & state_event_1) |
                             (state_event_0 & only_in_0_) |
                             (state_event_1 & only_in_1_);

    return state_event;
}

template<uint8_t NEvents, typename StorageIndex>
EventsSet<NEvents>
op::SuperProxy<NEvents, StorageIndex>::GetInvStateEvents(
  StorageIndex const& aQ) const
{
    auto const state_event_0 = sys0_.GetInvStateEvents(aQ % n_states_sys0_);
    auto const state_event_1 = sys1_.GetInvStateEvents(aQ / n_states_sys0_);
    auto const state_event = (state_event_0 & state_event_1) |
                             (state_event_0 & only_in_0_) |
                             (state_event_1 & only_in_1_);

    return state_event;
}

template<uint8_t NEvents, typename StorageIndex>
void
op::SuperProxy<NEvents, StorageIndex>::AllocateInvertedGraph() const
{
    sys0_.AllocateInvertedGraph();
    sys1_.AllocateInvertedGraph();
}

template<uint8_t NEvents, typename StorageIndex>
void
op::SuperProxy<NEvents, StorageIndex>::ClearInvertedGraph() const
{
    sys0_.ClearInvertedGraph();
    sys1_.ClearInvertedGraph();
}
}