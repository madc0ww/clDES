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

 File: desystem.hpp
 Description: DESystem class definition. DESystem is a graph, which is
 modeled as a Sparce Adjacency Matrix.
 =========================================================================
*/

#ifndef DESYSTEM_HPP
#define DESYSTEM_HPP

#ifndef VIENNACL_WITH_OPENCL
#define VIENNACL_WITH_OPENCL
#endif

#include "cldes/constants.hpp"
#include <Eigen/Sparse>
#include <QtCore/QPair>
#include <QtCore/QStack>
#include <set>
#include <stack>
#include <tuple>

namespace cldes {
/*
 * Forward declarion of DESystem class necessary for the forward declaration of
 * the DESystem's friend function op::Synchronize
 */
template<cldes::cldes_size_t NEvents = 32ul, typename StorageIndex = int>
class DESystem;

/*
 * Forward declarion of DESystem's friends class TransitionProxy. A transition
 * is an element of the adjascency matrix which implements the des graph.
 */
template<cldes::cldes_size_t NEvents, typename StorageIndex>
class TransitionProxy;

namespace op {
template<cldes::cldes_size_t NEvents, typename StorageIndex>
using GraphType =
  Eigen::SparseMatrix<std::bitset<NEvents>, Eigen::RowMajor, StorageIndex>;

/*
 * Forward declarion of DESystem's friend function Synchronize which
 * implements the parallel composition between two DES.
 */
template<cldes::cldes_size_t NEvents, typename StorageIndex>
cldes::DESystem<NEvents, StorageIndex>
Synchronize(DESystem<NEvents, StorageIndex> const& aSys0,
            DESystem<NEvents, StorageIndex> const& aSys1);

struct StatesTable;

struct StatesTuple;

using StatesTupleSTL = std::pair<cldes_size_t, cldes_size_t>;
using StatesTableSTL = QSet<cldes_size_t>;
using StatesStack = std::stack<cldes_size_t>;

template<cldes::cldes_size_t NEvents, typename StorageIndex>
cldes::DESystem<NEvents, StorageIndex>
SynchronizeStage1(cldes::DESystem<NEvents, StorageIndex> const& aSys0,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys1);

template<cldes::cldes_size_t NEvents, typename StorageIndex>
void
SynchronizeStage2(cldes::DESystem<NEvents, StorageIndex>& aVirtualSys,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys0,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys1);

template<cldes::cldes_size_t NEvents, typename StorageIndex>
cldes::cldes_size_t
TransitionVirtual(cldes::DESystem<NEvents, StorageIndex> const& aSys0,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys1,
                  cldes::cldes_size_t const& q,
                  cldes::ScalarType const& event);

template<cldes::cldes_size_t NEvents, typename StorageIndex>
void
RemoveBadStates(cldes::DESystem<NEvents, StorageIndex>& aVirtualSys,
                cldes::DESystem<NEvents, StorageIndex> const& aP,
                cldes::DESystem<NEvents, StorageIndex> const& aE,
                GraphType<NEvents, StorageIndex> const& aInvGraphP,
                GraphType<NEvents, StorageIndex> const& aInvGraphE,
                QSet<cldes_size_t>& C,
                cldes_size_t const& q,
                std::bitset<NEvents> const& s_non_contr,
                StatesTableSTL& rmtable);

template<cldes::cldes_size_t NEvents, typename StorageIndex>
cldes::DESystem<NEvents, StorageIndex>
SupervisorSynth(cldes::DESystem<NEvents, StorageIndex> const& aP,
                cldes::DESystem<NEvents, StorageIndex> const& aS,
                QSet<ScalarType> const& non_contr);
} // namespace op

template<cldes::cldes_size_t NEvents, typename StorageIndex>
class DESystem
{
public:
    using EventsSet = std::bitset<NEvents>;
    using GraphHostData =
      Eigen::SparseMatrix<EventsSet, Eigen::RowMajor, StorageIndex>;
    using BitGraphHostData =
      Eigen::SparseMatrix<bool, Eigen::RowMajor, StorageIndex>;
    using StatesSet = std::set<cldes_size_t>;
    using StatesVector =
      Eigen::SparseMatrix<bool, Eigen::ColMajor, StorageIndex>;
    using StatesEventsTable = std::vector<EventsSet>;

    /*! \brief DESystem constructor with empty matrix
     *
     * Overloads DESystem constructor: does not require to create a
     * eigen sparse matrix by the class user.
     *
     * @param aStatesNumber Number of states of the system
     * @param aInitState System's initial state
     * @param aMarkedStates System's marked states
     * @aDevCacheEnabled Enable or disable device cache for graph data
     */
    explicit DESystem(cldes_size_t const& aStatesNumber,
                      cldes_size_t const& aInitState,
                      StatesSet& aMarkedStates,
                      bool const& aDevCacheEnabled = true);

    // DESystem(DESystem &aSys);

    /*! \brief DESystem destructor
     *
     * Delete dinamically allocated data: graph and device_graph.
     */
    // virtual ~DESystem();

    /*! \brief Graph getter
     *
     * Returns a copy of DESystem's private data member graph. Considering that
     * graph is a pointer, it returns the contents of graph.
     */
    GraphHostData GetGraph() const;

    /*! \brief Returns state set containing the accessible part of automa
     *
     * Executes a Breadth First Search in the graph, which represents the DES,
     * starting from its initial state. It returns a set containing all nodes
     * which are accessible from the initial state.
     */
    StatesSet AccessiblePart();

    /*! \brief Returns state set containing the coaccessible part of automata
     *
     * Executes a Breadth First Search in the graph, until it reaches a marked
     * state.
     */
    StatesSet CoaccessiblePart();

    /*! \brief Returns States Set which is the Trim part of the system
     *
     * Gets the intersection between the accessible part and the coaccessible
     * part.
     */
    StatesSet TrimStates();

    /*! \brief Returns DES which is the Trim part of this
     *
     * Cut the non-accessible part of current system and then cut the
     * non-coaccessible part of the last result. The final resultant system
     * is called a trim system.
     *
     * @param aDevCacheEnabled Enables cache device graph on returned DES
     */
    void Trim();

    /*! \brief Returns value of the specified transition
     *
     * Override operator () for reading transinstions values:
     * e.g. discrete_system_foo(2,1);
     *
     * @param aLin Element's line
     * @param aCol Element's column
     */
    EventsSet const operator()(cldes_size_t const& aLin,
                               cldes_size_t const& aCol) const;

    /*! \brief Returns value of the specified transition
     *
     * Override operator () for changing transinstions with a single assignment:
     * e.g. discrete_system_foo(2,1) = 3.0f;
     *
     * @param aLin Element's line
     * @param aCol Element's column
     */
    TransitionProxy<NEvents, StorageIndex> operator()(cldes_size_t const& aLin,
                                                      cldes_size_t const& aCol);

    /*! \brief Returns number of states of the system
     *
     * Returns states_value_ by value.
     */
    cldes_size_t Size() const { return states_number_; }

    /*! \brief Set events_
     *
     * Set the member events_ with a set containing all events that are present
     * on the current system.
     */
    void InsertEvents(EventsSet const& aEvents);

    /*
     * TODO:
     * getters
     * enable dev cache
     * ...
     */
protected:
    /*! \brief Default constructor disabled
     *
     * Declare default constructor as protected to avoid the class user of
     * calling it.
     */
    DESystem(){};

private:
    friend class TransitionProxy<NEvents, StorageIndex>;
    friend DESystem cldes::op::Synchronize<NEvents, StorageIndex>(
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1);
    friend DESystem cldes::op::SynchronizeStage1<NEvents, StorageIndex>(
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1);
    friend void cldes::op::SynchronizeStage2<NEvents, StorageIndex>(
      DESystem<NEvents, StorageIndex>& aVirtualSys,
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1);
    friend cldes_size_t cldes::op::TransitionVirtual<NEvents, StorageIndex>(
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1,
      cldes_size_t const& q,
      ScalarType const& event);
    friend void cldes::op::RemoveBadStates<NEvents, StorageIndex>(
      DESystem<NEvents, StorageIndex>& aVirtualSys,
      DESystem<NEvents, StorageIndex> const& aP,
      DESystem<NEvents, StorageIndex> const& aE,
      op::GraphType<NEvents, StorageIndex> const& aInvGraphP,
      op::GraphType<NEvents, StorageIndex> const& aInvGraphE,
      QSet<cldes_size_t>& C,
      cldes_size_t const& q,
      std::bitset<NEvents> const& s_non_contr,
      QSet<cldes_size_t>& rmtable);
    friend DESystem cldes::op::SupervisorSynth<NEvents, StorageIndex>(
      DESystem<NEvents, StorageIndex> const& aP,
      DESystem<NEvents, StorageIndex> const& aE,
      QSet<ScalarType> const& non_contr);

    /*! \brief Graph represented by an adjascency matrix
     *
     * A sparse matrix who represents the automata as a graph in an
     * adjascency matrix. It is implemented as a CSR scheme. The pointer is
     * constant, but its content should not be constant, as the graph should
     * change many times at runtime.
     *
     */
    GraphHostData graph_;

    /*! \brief Bit graph represented by an adjascency matrix
     */
    BitGraphHostData bit_graph_;

    /*! \brief Keeps if caching graph data on device is enabled
     *
     * If dev_cache_enabled_ is true, the graph should be cached on the
     * device memory, so device_graph_ is not nullptr. It can be set at any
     * time at run time, so it is not a constant.
     */
    bool dev_cache_enabled_;

    /*! \brief Keeps track if the device graph cache is outdated
     *
     * Tracks if cache, dev_graph_, needs to be updated or not.
     */
    bool is_cache_outdated_;

    /*! \brief Current system's states number
     *
     * Hold the number of states that the automata contains. As the automata
     * can be cut, the states number is not a constant at all.
     */
    cldes_size_t states_number_;

    /*! \brief Current system's initial state
     *
     * Hold the initial state position.
     */
    cldes_size_t init_state_;

    /*! \brief Current system's marked states
     *
     * Hold all marked states. Cannot be const, since the automata can be
     * cut, and some marked states may be deleted.
     */
    StatesSet marked_states_;

    /*! \brief System's events hash table
     *
     * A hash table containing all the events that matter for the current
     * system.
     */
    EventsSet events_;

    /*! \brief Vector containing a events hash table per state
     */
    StatesEventsTable states_events_;

    /*! \brief data structures used in virtual systems
     */
    QList<cldes_size_t> virtual_states_;
    EventsSet only_in_0_;
    EventsSet only_in_1_;
    std::vector<
      std::pair<cldes_size_t, std::vector<std::pair<cldes_size_t, EventsSet>>>>
      transtriplet_;

    /*! \brief Vector containing a events hash table per state
     *
     * It represents the transitions of the inverted graph for the supervisor
     * synthesis.
     */
    StatesEventsTable inv_states_events_;

    /*! \brief Method for caching the graph
     *
     * Put graph transposed data on the device memory.
     */
    void CacheGraph_();

    /*! \brief Method for updating the graph
     *
     * Refresh the graph data on device memory.
     */
    void UpdateGraphCache_();

    /*! \brief Setup BFS and return accessed states array
     *
     * Executes a breadth first search on the graph starting from N nodes
     * in aInitialNodes. The algorithm is based on SpGEMM.
     *
     * @param aInitialNodes Set of nodes where the searches will start
     */
    template<class StatesType>
    StatesSet* Bfs_(StatesType const& aInitialNodes,
                    std::function<void(cldes_size_t const&,
                                       cldes_size_t const&)> const& aBfsVisit);
    StatesSet* Bfs_(cldes_size_t const& aInitialNode,
                    std::function<void(cldes_size_t const&,
                                       cldes_size_t const&)> const& aBfsVisit);

    /*! \brief Calculates Bfs and returns accessed states array
     *
     * Executes a breadth first search on the graph starting from one single
     * node. The algorithm is based on SpGEMM.
     *
     * @param aInitialNode Where the search will start
     */
    StatesSet* BfsCalc_(
      StatesVector& aHostX,
      std::function<void(cldes_size_t const&, cldes_size_t const&)> const&
        aBfsVisit,
      std::vector<cldes_size_t> const* const aStatesMap);

    /*! \brief Return a pointer to accessed states from the initial state
     *
     * Executes a breadth first search on the graph starting from
     * init_state_.
     */
    StatesSet* Bfs_();
};

} // namespace cldes

// Include DESystem implementation
#include "cldes/src/des/DESystemCore.hpp"

#endif // DESYSTEM_HPP