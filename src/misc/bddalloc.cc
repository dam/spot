#include <bdd.h>
#include <cassert>
#include "bddalloc.hh"

namespace spot
{
  bool bdd_allocator::initialized = false;
  int bdd_allocator::varnum = 2;

  bdd_allocator::bdd_allocator()
  {
    initialize();
    free_list.push_front(pos_lenght_pair(0, varnum));
  }

  void
  bdd_allocator::initialize()
  {
    if (initialized)
      return;
    initialized = true;
    bdd_init(50000, 5000);
    bdd_setvarnum(varnum);
  }

  int
  bdd_allocator::allocate_variables(int n)
  {
    // Browse the free list until we find N consecutive variables.  We
    // try not to fragment the list my allocating the variables in the
    // smallest free range we find.
    free_list_type::iterator best = free_list.end();
    free_list_type::iterator cur;
    for (cur = free_list.begin(); cur != free_list.end(); ++cur)
      {
	if (cur->second < n)
	  continue;
	if (n == cur->second)
	  {
	    best = cur;
	    break;
	  }
	if (best == free_list.end()
	    || cur->second < best->second)
	  best = cur;
      }

    // We have found enough free variables.
    if (best != free_list.end())
      {
	int result = best->first;
	best->second -= n;
	assert(best->second >= 0);
	// Erase the range if it's now empty.
	if (best->second == 0)
	  free_list.erase(best);
	else
	  best->first += n;
	return result;
      }

    // We haven't found enough adjacent free variables;
    // ask BuDDy for some more.

    // If we already have some free variable at the end
    // of the variable space, allocate just the difference.
    if (free_list.size() > 0
	&& free_list.back().first + free_list.back().second == varnum)
      {
	int res = free_list.back().first;
	int endvar = free_list.back().second;
	assert(n > endvar);
	bdd_extvarnum(n - endvar);
	varnum += n - endvar;
	free_list.pop_back();
	return res;
      }
    else
      {
	// Otherwise, allocate as much variables as we need.
	int res = varnum;
	bdd_extvarnum(n);
	varnum += n;
	return res;
      }
  }

  void
  bdd_allocator::release_variables(int base, int n)
  {
    free_list_type::iterator cur;
    int end = base + n;
    for (cur = free_list.begin(); cur != free_list.end(); ++cur)
      {
	// Append to a range ...
	if (cur->first + cur->second == base)
	  {
	    cur->second += n;
	    // Maybe the next item on the list can be merged.
	    free_list_type::iterator next = cur;
	    ++next;
	    if (next != free_list.end()
		&& next->first == end)
	      {
		cur->second += next->second;
		free_list.erase(next);
	      }
	    return;
	  }
	// ... or prepend to a range ...
	if (cur->first == end)
	  {
	    cur->first -= n;
	    cur->second += n;
	    return;
	  }
	// ... or insert a new range.
	if (cur->first > end)
	  break;
      }
    free_list.insert(cur, pos_lenght_pair(base, n));
  }
}