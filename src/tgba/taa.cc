// Copyright (C) 2009 Laboratoire d'Informatique de Paris
// 6 (LIP6), département Systèmes Répartis Coopératifs (SRC),
// Université Pierre et Marie Curie.
//
// This file is part of Spot, a model checking library.
//
// Spot is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Spot is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Spot; see the file COPYING.  If not, write to the Free
// Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#include <map>
#include <algorithm>
#include <iostream>
#include "ltlvisit/destroy.hh"
#include "tgba/formula2bdd.hh"
#include "misc/bddop.hh"
#include "taa.hh"

namespace spot
{
  /*----.
  | taa |
  `----*/

  taa::taa(bdd_dict* dict)
    : name_state_map_(), state_name_map_(), dict_(dict),
      all_acceptance_conditions_(bddfalse),
      all_acceptance_conditions_computed_(false),
      neg_acceptance_conditions_(bddtrue),
      init_(0), state_set_vec_()
  {
  }

  taa::~taa()
  {
    ns_map::iterator i;
    for (i = name_state_map_.begin(); i != name_state_map_.end(); ++i)
    {
      taa::state::iterator i2;
      for (i2 = i->second->begin(); i2 != i->second->end(); ++i2)
	delete *i2;
      delete i->second;
    }
    ss_vec::iterator j;
    for (j = state_set_vec_.begin(); j != state_set_vec_.end(); ++j)
      delete *j;
    dict_->unregister_all_my_variables(this);
  }

  void
  taa::set_init_state(const std::string& s)
  {
    init_ = add_state(s);
  }

  taa::transition*
  taa::create_transition(const std::string& s,
			 const std::vector<std::string>& d)
  {
    state* src = add_state(s);
    state_set* dst = add_state_set(d);
    transition* t = new transition;
    t->dst = dst;
    t->condition = bddtrue;
    t->acceptance_conditions = bddfalse;
    src->push_back(t);
    return t;
  }

  taa::transition*
  taa::create_transition(const std::string& s, const std::string& d)
  {
    std::vector<std::string> vec;
    vec.push_back(d);
    return create_transition(s, vec);
  }

  void
  taa::add_condition(transition* t, const ltl::formula* f)
  {
    t->condition &= formula_to_bdd(f, dict_, this);
    ltl::destroy(f);
  }

  void
  taa::add_acceptance_condition(transition* t, const ltl::formula* f)
  {
    if (dict_->acc_map.find(f) == dict_->acc_map.end())
    {
      int v = dict_->register_acceptance_variable(f, this);
      bdd neg = bdd_nithvar(v);
      neg_acceptance_conditions_ &= neg;

      // Append neg to all acceptance conditions.
      ns_map::iterator i;
      for (i = name_state_map_.begin(); i != name_state_map_.end(); ++i)
      {
	taa::state::iterator i2;
	for (i2 = i->second->begin(); i2 != i->second->end(); ++i2)
	  (*i2)->acceptance_conditions &= neg;
      }

      all_acceptance_conditions_computed_ = false;
    }

    bdd_dict::fv_map::iterator i = dict_->acc_map.find(f);
    assert(i != dict_->acc_map.end());
    ltl::destroy(f);
    bdd v = bdd_ithvar(i->second);
    t->acceptance_conditions |= v & bdd_exist(neg_acceptance_conditions_, v);
  }

  void
  taa::output(std::ostream& os) const
  {
    ns_map::const_iterator i;
    for (i = name_state_map_.begin(); i != name_state_map_.end(); ++i)
    {
      taa::state::const_iterator i2;
      os << "State: " << i->first << std::endl;
      for (i2 = i->second->begin(); i2 != i->second->end(); ++i2)
      {
	os << " " << format_state_set((*i2)->dst)
	   << ", C:" << (*i2)->condition
	   << ", A:" << (*i2)->acceptance_conditions << std::endl;
      }
    }
  }

  state*
  taa::get_init_state() const
  {
    taa::state_set* s = new taa::state_set;
    s->insert(init_);
    return new spot::state_set(s);
  }

  tgba_succ_iterator*
  taa::succ_iter(const spot::state* state,
		 const spot::state* global_state,
		 const tgba* global_automaton) const
  {
    const spot::state_set* s = dynamic_cast<const spot::state_set*>(state);
    assert(s);
    (void) global_state;
    (void) global_automaton;
    return new taa_succ_iterator(s->get_state(),
				 all_acceptance_conditions());
  }

  bdd_dict*
  taa::get_dict() const
  {
    return dict_;
  }

  std::string
  taa::format_state(const spot::state* s) const
  {
    const spot::state_set* se = dynamic_cast<const spot::state_set*>(s);
    assert(se);
    const state_set* ss = se->get_state();
    return format_state_set(ss);
  }

  bdd
  taa::all_acceptance_conditions() const
  {
    if (!all_acceptance_conditions_computed_)
    {
      all_acceptance_conditions_ =
	compute_all_acceptance_conditions(neg_acceptance_conditions_);
      all_acceptance_conditions_computed_ = true;
    }
    return all_acceptance_conditions_;
  }

  bdd
  taa::neg_acceptance_conditions() const
  {
    return neg_acceptance_conditions_;
  }

  bdd
  taa::compute_support_conditions(const spot::state* s) const
  {
    const spot::state_set* se = dynamic_cast<const spot::state_set*>(s);
    assert(se);
    const state_set* ss = se->get_state();

    bdd res = bddtrue;
    taa::state_set::const_iterator i;
    taa::state::const_iterator j;
    for (i = ss->begin(); i != ss->end(); ++i)
      for (j = (*i)->begin(); j != (*i)->end(); ++j)
	res |= (*j)->condition;
    return res;
  }

  bdd
  taa::compute_support_variables(const spot::state* s) const
  {
    const spot::state_set* se = dynamic_cast<const spot::state_set*>(s);
    assert(se);
    const state_set* ss = se->get_state();

    bdd res = bddtrue;
    taa::state_set::const_iterator i;
    taa::state::const_iterator j;
    for (i = ss->begin(); i != ss->end(); ++i)
      for (j = (*i)->begin(); j != (*i)->end(); ++j)
	res &= bdd_support((*j)->condition);
    return res;
  }

  taa::state*
  taa::add_state(const std::string& name)
  {
    ns_map::iterator i = name_state_map_.find(name);
    if (i == name_state_map_.end())
    {
      taa::state* s = new taa::state;
      name_state_map_[name] = s;
      state_name_map_[s] = name;

      // The first state we add is the inititial state.
      // It can also be overridden with set_init_state().
      if (!init_)
	init_ = s;

      return s;
    }
    return i->second;
  }

  taa::state_set*
  taa::add_state_set(const std::vector<std::string>& names)
  {
    state_set* ss = new state_set;
    for (unsigned i = 0; i < names.size(); ++i)
      ss->insert(add_state(names[i]));
    state_set_vec_.push_back(ss);
    return ss;
  }

  std::string
  taa::format_state_set(const taa::state_set* ss) const
  {
    state_set::const_iterator i1 = ss->begin();
    sn_map::const_iterator i2;
    if (ss->empty())
      return std::string("{}");
    if (ss->size() == 1)
    {
      i2 = state_name_map_.find(*i1);
      assert(i2 != state_name_map_.end());
      return "{" + i2->second + "}";
    }
    else
    {
      std::string res("{");
      while (i1 != ss->end())
      {
	i2 = state_name_map_.find(*i1++);
	assert(i2 != state_name_map_.end());
	res += i2->second;
	res += ",";
      }
      res[res.size() - 1] = '}';
      return res;
    }
  }

  /*----------.
  | state_set |
  `----------*/

  const taa::state_set*
  state_set::get_state() const
  {
    return s_;
  }

  int
  state_set::compare(const spot::state* other) const
  {
    const state_set* o = dynamic_cast<const state_set*>(other);
    assert(o);

    const taa::state_set* s1 = get_state();
    const taa::state_set* s2 = o->get_state();

    if (*s1 == *s2)
      return 0;

    taa::state_set::const_iterator it1 = s1->begin();
    taa::state_set::const_iterator it2 = s2->begin();
    while (it2 != s2->end() && it1 != s1->end())
    {
      int i = *it1++ - *it2++;
      if (i != 0)
	return i;
    }
    return s1->size() - s2->size();
  }

  size_t
  state_set::hash() const
  {
    size_t res = wang32_hash(0);
    taa::state_set::const_iterator it = s_->begin();
    while (it != s_->end())
    {
      res += reinterpret_cast<const char*>(*it++) - static_cast<char*>(0);
      res ^= wang32_hash(res);
    }
    return res;
  }

  state_set*
  state_set::clone() const
  {
    taa::state_set* s = new taa::state_set(*s_);
    return new spot::state_set(s);
  }

  /*--------------.
  | taa_succ_iter |
  `--------------*/

  taa_succ_iterator::taa_succ_iterator(const taa::state_set* s, bdd all_acc)
    : all_acceptance_conditions_(all_acc), seen_()
  {
    if (s->empty())
    {
      taa::transition* t = new taa::transition;
      t->condition = bddtrue;
      t->acceptance_conditions = bddfalse;
      t->dst = new taa::state_set;
      succ_.push_back(t);
      return;
    }

    std::vector<iterator> pos;
    std::vector<std::pair<iterator, iterator> > bounds;
    for (taa::state_set::const_iterator i = s->begin(); i != s->end(); ++i)
    {
      pos.push_back((*i)->begin());
      bounds.push_back(std::make_pair((*i)->begin(), (*i)->end()));
    }

    while (pos[0] != bounds[0].second)
    {
      taa::transition* t = new taa::transition;
      t->condition = bddtrue;
      t->acceptance_conditions = bddfalse;
      taa::state_set* ss = new taa::state_set;
      for (unsigned i = 0; i < pos.size(); ++i)
      {
	taa::state_set::const_iterator j;
	for (j = (*pos[i])->dst->begin(); j != (*pos[i])->dst->end(); ++j)
	  if ((*j)->size() > 0) // Remove well states.
	    ss->insert(*j);

	// Fill the new transition.
	t->dst = ss;
	t->condition &= (*pos[i])->condition;
	t->acceptance_conditions |= (*pos[i])->acceptance_conditions;
      }
      // Look for another transition to merge with.
      seen_map::iterator i;
      for (i = seen_.find(*ss); i != seen_.end(); ++i)
      {
	if (*i->second->dst == *t->dst
	    && i->second->condition == t->condition)
	{
	  i->second->acceptance_conditions &= t->acceptance_conditions;
	  break;
	}
	if (*i->second->dst == *t->dst
	    && i->second->acceptance_conditions == t->acceptance_conditions)
	{
	  i->second->condition |= t->condition;
	  break;
	}
      }
      // Mark this transition as seen and keep it, or delete it.
      if (i == seen_.end() && t->condition != bddfalse)
      {
	seen_.insert(std::make_pair(*ss, t));
	succ_.push_back(t);
      }
      else
      {
	delete t->dst;
	delete t;
      }

      for (int i = pos.size() - 1; i >= 0; --i)
      {
	if (std::distance(pos[i], bounds[i].second) > 1 ||
	    (i == 0 && std::distance(pos[i], bounds[i].second) == 1))
	{
	  ++pos[i];
	  break;
	}
	else
	  pos[i] = bounds[i].first;
      }
    }
  }

  taa_succ_iterator::~taa_succ_iterator()
  {
    for (unsigned i = 0; i < succ_.size(); ++i)
      delete succ_[i];
  }

  void
  taa_succ_iterator::first()
  {
    i_ = succ_.begin();
  }

  void
  taa_succ_iterator::next()
  {
    ++i_;
  }

  bool
  taa_succ_iterator::done() const
  {
    return i_ == succ_.end();
  }

  spot::state_set*
  taa_succ_iterator::current_state() const
  {
    assert(!done());
    return new spot::state_set((*i_)->dst);
  }

  bdd
  taa_succ_iterator::current_condition() const
  {
    assert(!done());
    return (*i_)->condition;
  }

  bdd
  taa_succ_iterator::current_acceptance_conditions() const
  {
    assert(!done());
    return all_acceptance_conditions_ -
      ((*i_)->acceptance_conditions & all_acceptance_conditions_);
  }
}