#include <set>
#include "tgba/tgba.hh"
#include "save.hh"
#include "tgba/bddprint.hh"

namespace spot
{
  typedef std::set<state*, state_ptr_less_than> seen_set;

  /// Process successors.
  static void
  save_rec(std::ostream& os, const tgba& g, state* st, seen_set& m)
  {
    m.insert(st);
    std::string cur = g.format_state(st);
    tgba_succ_iterator* si = g.succ_iter(st);
    for (si->first(); !si->done(); si->next())
      {
	state* s = si->current_state();
	os << "\"" << cur << "\", \"" << g.format_state(s) << "\", ";

	bdd_print_sat(os, g.get_dict(), si->current_condition()) << ", ";
	bdd_print_sat(os, g.get_dict(), si->current_promise()) << ";"
							       << std::endl;

	// Destination already explored?
	seen_set::iterator i = m.find(s);
	if (i != m.end())
	  {
	    delete s;
	  }
	else
	  {
	    save_rec(os, g, s, m);
	    // Do not delete S, it is used as key in M.
	  }
      }
    delete si;
  }

  std::ostream&
  tgba_save_reachable(std::ostream& os, const tgba& g)
  {
    seen_set m;
    state* state = g.get_init_state();
    save_rec(os, g, state, m);
    // Finally delete all states used as keys in m:
    for (seen_set::iterator i = m.begin(); i != m.end(); ++i)
      delete *i;
    return os;
  }
}