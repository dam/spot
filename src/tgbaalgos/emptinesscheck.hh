#ifndef SPOT_EMPTINESS_CHECK_HH
# define SPOT_EMPTINESS_CHECK_HH

#include "tgba/tgba.hh"
#include "misc/hash.hh"
#include <stack>
#include <list>
#include <utility>
#include <iostream>

namespace spot
{

  class connected_component
  {
    // During the Depth path we keep the connected component that we met.
  public:
    connected_component(int index = -1);

  public:
    int index;
    /// The bdd condition is the union of all accepting condition of
    /// transitions which connect the states of the connected component.
    bdd condition;

    typedef Sgi::hash_set<const state*,
			  state_ptr_hash, state_ptr_equal> set_type;
    /// for the counter example we need to know all the states of the
    /// component
    set_type state_set;

    /// Check if the SCC contains states \a s.
    bool has_state(const state* s) const;
  };

  /// \brief Check whether the language of an automate is empty.
  ///
  /// This is based on the following paper.
  /// \verbatim
  /// @InProceedings{couvreur.99.fm,
  ///   author    = {Jean-Michel Couvreur},
  ///   title     = {On-the-fly Verification of Temporal Logic},
  ///   pages     = {253--271},
  ///   editor    = {Jeannette M. Wing and Jim Woodcock and Jim Davies},
  ///   booktitle = {Proceedings of the World Congress on Formal Methods in
  ///                the Development of Computing Systems (FM'99)},
  ///   publisher = {Springer-Verlag},
  ///   series    = {Lecture Notes in Computer Science},
  ///   volume    = {1708},
  ///   year      = {1999},
  ///   address   = {Toulouse, France},
  ///   month     = {September},
  ///   isbn      = {3-540-66587-0}
  /// }
  /// \endverbatim
  class emptiness_check
  {
    typedef std::list<const state*> state_sequence;
    typedef std::pair<const state*, bdd> state_proposition;
    typedef std::list<state_proposition> cycle_path;
  public:
    emptiness_check(const tgba* a);

    /// This function returns true if the automata's language is empty,
    /// and builds a stack of SCC.
    bool check();

    /// Compute a counter example if tgba_emptiness_check() returned false.
    void counter_example();

    std::ostream& print_result(std::ostream& os,
			       const tgba* restrict = 0) const;

  private:
    const tgba* aut_;
    std::stack<connected_component> root_component;
    state_sequence suffix;
    cycle_path period;

    typedef Sgi::hash_map<const state*, int,
			  state_ptr_hash, state_ptr_equal> hash_type;
    hash_type h;		///< Map of visited states.

    /// \brief Remove a strongly component from the hash.
    ///
    /// This function remove all accessible state from a given
    /// state. In other words, it removes the strongly connected
    /// component that contains this state.
    void remove_component(const state* start_delete);

    /// Called by counter_example to find a path which traverses all
    /// accepting conditions in the accepted SCC.
    void accepting_path (const connected_component& comp_path,
			 const state* start_path, bdd to_accept);

    /// Complete a cycle that caraterise the period of the counter
    /// example.  Append a sequence to the path given by accepting_path.
    void complete_cycle(const connected_component& comp_path,
			const state* from_state,const state* to_state);
  };
}
#endif // SPOT_EMPTINESS_CHECK_HH