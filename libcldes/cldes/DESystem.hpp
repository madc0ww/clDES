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

 File: cldes/DESystem.hpp
 Description: DESystem class declaration. DESystem is a graph, which is
 modeled as a Sparce Adjacency Matrix.
 =========================================================================
*/

#ifndef DESYSTEM_HPP
#define DESYSTEM_HPP

#include "cldes/DESystemBase.hpp"
#include <sparsepp/spp.h>
#include <stack>
#include <tuple>

/*
 * Forward declarations and some useful alias definitions
 */
namespace cldes {
/*! \brief Vector that contains arguments of inverse transition function
 *
 * V f(s_from, e) = s_to |-> f^-1(s_to, e) = s_from
 * f^-1 args are (s_to, e)
 */
template<typename StorageIndex>
using InvArgTrans = std::vector<std::pair<StorageIndex, cldes::ScalarType>>;

/*! \brief Hash map containing transitions
 *
 * V f(s_from, e) = s_to |-> f^-1(s_to, e) = s_from
 * {key = s_from, value= vec(s_from, e))
 */
template<typename StorageIndex>
using TransMap = spp::sparse_hash_map<StorageIndex, InvArgTrans<StorageIndex>*>;

/*
 * Forward declarion of DESystem class necessary for the forward declaration of
 * the DESystem's friend function op::Synchronize
 */
template<uint8_t NEvents = 32u, typename StorageIndex = unsigned>
class DESystem;

// Forward declartions of friends functions which implement des operations
namespace op {
template<uint8_t NEvents>
using GraphType = Eigen::SparseMatrix<EventsSet<NEvents>, Eigen::RowMajor>;

/*
 * Forward declarion of DESystem's friend function Synchronize which
 * implements the parallel composition between two DES.
 */
template<uint8_t NEvents, typename StorageIndex>
cldes::DESystem<NEvents, StorageIndex>
Synchronize(DESystem<NEvents, StorageIndex> const& aSys0,
            DESystem<NEvents, StorageIndex> const& aSys1);

// Table containing states of a system on the device mem
struct StatesTable;

/*
 * Tuple which implements the virtual states of sync a system on the device
 * memory
 */
struct StatesTuple;

// Tuple representing a virtual state
template<typename StorageIndex>
using StatesTupleHost = std::pair<StorageIndex, StorageIndex>;

// Table containing virtual states of a virtual system as integers/longs
template<typename StorageIndex>
using StatesTableHost = spp::sparse_hash_set<StorageIndex>;

// Stack of states for implementing a DFS
template<typename StorageIndex>
using StatesStack = std::stack<StorageIndex>;

// Table of events
using EventsTableHost = spp::sparse_hash_set<uint8_t>;

// Implements the first stage of the parallel composition: gen tuples
template<uint8_t NEvents, typename StorageIndex>
cldes::DESystem<NEvents, StorageIndex>
SynchronizeStage1(cldes::DESystem<NEvents, StorageIndex> const& aSys0,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys1);

// Implements the second stage of the parallel composition: calc transitions
template<uint8_t NEvents, typename StorageIndex>
void
SynchronizeStage2(cldes::DESystem<NEvents, StorageIndex>& aVirtualSys,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys0,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys1);

// Calculate f(q, event) of a virtual system
template<uint8_t NEvents, typename StorageIndex>
StorageIndex
TransitionVirtual(cldes::DESystem<NEvents, StorageIndex> const& aSys0,
                  cldes::DESystem<NEvents, StorageIndex> const& aSys1,
                  StorageIndex const& aQ,
                  cldes::ScalarType const& aEvent);

// Remove bad states recursively
template<uint8_t NEvents, typename StorageIndex>
void
RemoveBadStates(cldes::DESystem<NEvents, StorageIndex>& aVirtualSys,
                cldes::DESystem<NEvents, StorageIndex> const& aP,
                cldes::DESystem<NEvents, StorageIndex> const& aE,
                GraphType<NEvents> const& aInvGraphP,
                GraphType<NEvents> const& aInvGraphE,
                TransMap<StorageIndex>& aC,
                StorageIndex const& aQ,
                EventsSet<NEvents> const& aNonContrBit,
                StatesTableHost<StorageIndex>& aRmTable);

// Calculate the monolithic supervisor of a fiven plant and spec
template<uint8_t NEvents, typename StorageIndex>
cldes::DESystem<NEvents, StorageIndex>
SupervisorSynth(cldes::DESystem<NEvents, StorageIndex> const& aP,
                cldes::DESystem<NEvents, StorageIndex> const& aS,
                EventsTableHost const& aNonContr);
} // namespace op

/*! \brief Discrete-Events System on host memory
 *
 * Implement a DES on the host memory and their respective operations for CPUs.
 *
 * @param NEvents Number of events
 * @param StorageIndex Unsigned type use for indexing the ajacency matrix
 */
template<uint8_t NEvents, typename StorageIndex>
class DESystem : public DESystemBase<NEvents, StorageIndex>
{
public:
    /*! \brief Adjacency matrix of bitarrays implementing a graph
     *
     * The graph represents the DES automata:
     * Non zero element: contains at least one transition
     * Each non 0 bit of each element: event that lead to the next stage
     *
     * row index: from state
     * col index: to state
     */
    using GraphHostData =
      typename DESystemBase<NEvents, StorageIndex>::GraphHostData;

    /*! \brief Set of states type
     */
    using StatesSet = typename DESystemBase<NEvents, StorageIndex>::StatesSet;

    /*! \brief Table of transitions on a STL container
     */
    using StatesEventsTable =
      typename DESystemBase<NEvents, StorageIndex>::StatesSet;

    /*! \brief Adjacency matrix of bit implementing a graph
     *
     * Sparse matrix implementing a graph with an adjacency matrix.
     * The graph represents a simplified DES automata:
     * Non zero element: contains at least one transition
     *
     * row index: from state
     * col index: to state
     */
    using BitGraphHostData = Eigen::SparseMatrix<bool, Eigen::RowMajor>;

    /*! \brief Adjacency  matrix of bit implementing searching nodes
     *
     * Structure used for traversing the graph using a linear algebra approach
     */
    using StatesVector = Eigen::SparseMatrix<bool, Eigen::ColMajor>;

    /*! \brief Set of Events implemented as a Hash Table for searching
     * efficiently.
     */
    using EventsTable = spp::sparse_hash_set<uint8_t>;

    /*! \brief Vector of states type
     */
    using StatesTable = std::vector<StorageIndex>;

    /*! \brief Vector of inverted transitions
     *
     * f(s, e) = s_out -> (s_out, (s, e)) is the inverted transition.
     */
    using TrVector =
      std::vector<std::pair<StorageIndex, InvArgTrans<StorageIndex>*>>;

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
    explicit DESystem(StorageIndex const& aStatesNumber,
                      StorageIndex const& aInitState,
                      StatesSet& aMarkedStates,
                      bool const& aDevCacheEnabled = true);

    /*! \brief Move constructor
     *
     * Enable move semantics
     */
    DESystem(DESystem&&) = default;

    /*! \brief Copy constructor
     *
     * Needs to define this, since move semantics is enabled
     */
    DESystem(DESystem const&) = default;

    /*! \brief Operator =
     *
     * Uses move semantics
     */
    DESystem<NEvents, StorageIndex>& operator=(DESystem&&) = default;

    /*! \brief Operator = to const type
     *
     * Needs to define this, since move semantics is enabled
     */
    DESystem<NEvents, StorageIndex>& operator=(DESystem const&) = default;

    /*! \brief DESystem destructor
     */
    virtual ~DESystem() = default;

    /*! \brief Returns state set containing the accessible part of automa
     *
     * Executes a Breadth First Search in the graph, which represents the DES,
     * starting from its initial state. It returns a set containing all nodes
     * which are accessible from the initial state.
     */
    StatesSet AccessiblePart() override;

    /*! \brief Returns state set containing the coaccessible part of automata
     *
     * Executes a Breadth First Search in the graph, until it reaches a marked
     * state.
     */
    StatesSet CoaccessiblePart() override;

    /*! \brief Returns States Set which is the Trim part of the system
     *
     * Gets the intersection between the accessible part and the coaccessible
     * part.
     */
    StatesSet TrimStates() override;

    /*! \brief Returns DES which is the Trim part of this
     *
     * Cut the non-accessible part of current system and then cut the
     * non-coaccessible part of the last result. The final resultant system
     * is called a trim system.
     *
     * @param aDevCacheEnabled Enables cache device graph on returned DES
     */
    void Trim() override;

    /*! \brief Insert events
     *
     * Set the member events_ with a set containing all events that are present
     * on the current system. Take care using this method. It was designed for
     * testing and debugging.
     *
     * @params aEvents Set containing all the new events of the current system
     */
    void InsertEvents(EventsSet<NEvents> const& aEvents) override;

    /*
     * TODO:
     * getters
     * setters: e.g. remove transition, remove state
     * enable dev cache
     * ...
     */
protected:
    /*! \brief Default constructor disabled
     *
     * Declare default constructor as protected to avoid the class user of
     * calling it.
     */
    explicit DESystem(){};

    /*! \brief Let the derived class know that a new element was inserted
     *
     * @param aQfrom State from
     * @param aQto State to
     */
    void NewTransition_(StorageIndex const& aQfrom,
                        StorageIndex const& aQto) override;

private:
    /* DESystem operations
     *
     * Functions which implement DES operations between two systems and need
     * to access private members of DESystem. The design constrains that lead to
     * declare all these friends functions are:
     *
     * * Functions which have more than one system as input and returns a
     * different system should not be a member of DESystem class.
     * * More efficiency when accessing vars than using getters and setters.
     * * Define a different namespace for DES operations.
     */
    // Parallel composition
    friend DESystem cldes::op::Synchronize<>(
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1);

    // First step of the lazy parallel composition
    friend DESystem cldes::op::SynchronizeStage1<>(
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1);

    // Second step of the lazy parallel composition
    friend void cldes::op::SynchronizeStage2<>(
      DESystem<NEvents, StorageIndex>& aVirtualSys,
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1);

    // Calculate f(q, event) of a virtual system
    friend StorageIndex cldes::op::TransitionVirtual<>(
      DESystem<NEvents, StorageIndex> const& aSys0,
      DESystem<NEvents, StorageIndex> const& aSys1,
      StorageIndex const& aQ,
      cldes::ScalarType const& aEvent);

    // Remove a bad state recursively from a virtualsys
    friend void cldes::op::RemoveBadStates<NEvents, StorageIndex>(
      DESystem<NEvents, StorageIndex>& aVirtualSys,
      DESystem<NEvents, StorageIndex> const& aP,
      DESystem<NEvents, StorageIndex> const& aE,
      op::GraphType<NEvents> const& aInvGraphP,
      op::GraphType<NEvents> const& aInvGraphE,
      TransMap<StorageIndex>& aC,
      StorageIndex const& aQ,
      EventsSet<NEvents> const& aNonContr,
      op::StatesTableHost<StorageIndex>& aRmTable);

    // Computes the monolithic supervisor
    friend DESystem cldes::op::SupervisorSynth<>(
      DESystem<NEvents, StorageIndex> const& aP,
      DESystem<NEvents, StorageIndex> const& aE,
      op::EventsTableHost const& aNonContr);

    /*! \brief Graph represented by an adjascency matrix
     *
     * A sparse bit matrix which each non zero elem represents that a state
     * has at least one transition to other state.
     * It is used to calculate the accessible part, coaccessible part and trim
     * operations efficiently.
     *
     * The matrix is also transposed and added to the identity in order to
     * make the accessible part op more efficient:
     * when calculating trim states, it is always necessary to calculate the
     * accessible part first. It implies that the accessible part usually is
     * calculated with a larger matrix. Adding the identity avoid to add two
     * sparse matrices each bfs iteration, which is inneficient.
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

    /*! \brief Virtual states contained in the current system
     *
     * Valid onnly when this system is virtual.
     * TODO: Change it to a pointer and allocate only when the system is virtual
     * and deallocate when it become a concrete system.
     */
    StatesTable virtual_states_;

    /*! \brief Events contained only in the left operator of a synchronizing op.
     *
     * Valid onnly when this system is virtual.
     * TODO: Change it to a pointer and allocate only when the system is virtual
     * and deallocate when it become a concrete system.
     */
    EventsSet<NEvents> only_in_0_;

    /*! \brief Events contained only in the right operator of a synchronizing
     * op.
     *
     * Valid onnly when this system is virtual.
     * TODO: Change it to a pointer and allocate only when the system is virtual
     * and deallocate when it become a concrete system.
     */
    EventsSet<NEvents> only_in_1_;

    /*! \brief Events contained only in the right operator of a synchronizing
     * op.
     *
     * Valid onnly when this system is virtual.
     * TODO: Change it to a pointer and allocate only when the system is virtual
     * and deallocate when it become a concrete system.
     */
    TrVector transtriplet_;

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
     * @param aBfsVisit Function to execute on every accessed state: it can be
     * null
     */
    template<class StatesType>
    StatesSet* Bfs_(StatesType const& aInitialNodes,
                    std::function<void(StorageIndex const&,
                                       StorageIndex const&)> const& aBfsVisit);

    /*! \brief Overload Bfs_ for the special case of a single initial node
     */
    StatesSet* Bfs_(StorageIndex const& aInitialNode,
                    std::function<void(StorageIndex const&,
                                       StorageIndex const&)> const& aBfsVisit);

    /*! \brief Calculates Bfs and returns accessed states array
     *
     * Executes a breadth first search on the graph starting from one single
     * node. The algorithm is based on SpGEMM.
     *
     * @param aInitialNode Where the search will start
     * @param aBfsVisit Function to execute on every accessed state: it can be
     * null
     * @param aStatesMap Map each vector used to implement in a bfs when
     * multiple ones are execute togheter: it can be null
     */
    StatesSet* BfsCalc_(
      StatesVector& aHostX,
      std::function<void(StorageIndex const&, StorageIndex const&)> const&
        aBfsVisit,
      std::vector<StorageIndex> const* const aStatesMap);

    /*! \brief Return a pointer to accessed states from the initial state
     *
     * Executes a breadth first search on the graph starting from
     * init_state_.
     */
    StatesSet* Bfs_();
};

/*! \brief Alias for bit graph 3-tuple
 *
 * (s_from, s_to, true)
 */
using BitTriplet = Eigen::Triplet<bool>;
} // namespace cldes

// Include DESystem implementation
#include "cldes/src/des/DESystemCore.hpp"

#endif // DESYSTEM_HPP
