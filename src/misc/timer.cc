// Copyright (C) 2004  Laboratoire d'Informatique de Paris 6 (LIP6),
// d�partement Syst�mes R�partis Coop�ratifs (SRC), Universit� Pierre
// et Marie Curie.
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

#include "timer.hh"
#include <iostream>
#include <iomanip>

namespace spot
{

  std::ostream&
  timer_map::print(std::ostream& os) const
  {
    time_info total;
    for (tm_type::const_iterator i = tm.begin(); i != tm.end(); ++i)
      {
	total.utime += i->second.first.utime();
	total.stime += i->second.first.stime();
      }
    clock_t grand_total = total.utime + total.stime;

    os << std::setw(33) << ""
       << "|  user time |  sys. time |    total   |"
       << std::endl
       << std::setw(33) << "name "
       << "| ticks    % | tics     % | tics     % |   n"
       << std::endl
       << std::setw(79) << std::setfill('-') << "" << std::setfill(' ')
       << std::endl;
    for (tm_type::const_iterator i = tm.begin(); i != tm.end(); ++i)
      {
	const spot::timer& t = i->second.first;
	os << std::setw(32) << i->first << " |"
	   << std::setw(6) << t.utime()
	   << std::setw(5) << (total.utime ?
			       100.0 * t.utime() / total.utime : 0)
	   << " |"
	   << std::setw(6) << t.stime()
	   << std::setw(5) << (total.stime ?
			       100.0 * t.stime() / total.stime : 0)
	   << " |"
	   << std::setw(6) << t.utime() + t.stime()
	   << std::setw(5) << (grand_total ?
			       (100.0 * (t.utime() + t.stime()) /
				grand_total) : 0)
	   << " |"
	   << std::setw(4) << i->second.second
	   << std::endl;
      }
    os << std::setw(79) << std::setfill('-') << "" << std::setfill(' ')
       << std::endl
       << std::setw(32) << "TOTAL" << " |"
       << std::setw(6) << total.utime
       << std::setw(5) << 100
       << " |"
       << std::setw(6) << total.stime
       << std::setw(5) << 100
       << " |"
       << std::setw(6) << grand_total
       << std::setw(5) << 100
       << " |"
       << std::endl;
    return os;
  }

}