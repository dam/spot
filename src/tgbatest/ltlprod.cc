#include <iostream>
#include <cassert>
#include "ltlvisit/destroy.hh"
#include "ltlast/allnodes.hh"
#include "ltlparse/public.hh"
#include "tgba/ltl2tgba.hh"
#include "tgba/tgbabddprod.hh"
#include "tgba/tgbabddconcreteproduct.hh"
#include "tgbaalgos/dotty.hh"

void
syntax(char* prog)
{
  std::cerr << prog << " formula1 formula2" << std::endl;
  exit(2);
}

int
main(int argc, char** argv)
{
  int exit_code = 0;

  if (argc != 3)
    syntax(argv[0]);

  spot::ltl::environment& env(spot::ltl::default_environment::instance());

  spot::ltl::parse_error_list pel1;
  spot::ltl::formula* f1 = spot::ltl::parse(argv[1], pel1, env);

  if (spot::ltl::format_parse_errors(std::cerr, argv[1], pel1))
    return 2;

  spot::ltl::parse_error_list pel2;
  spot::ltl::formula* f2 = spot::ltl::parse(argv[2], pel2, env);

  if (spot::ltl::format_parse_errors(std::cerr, argv[2], pel2))
    return 2;

  {
    spot::tgba_bdd_concrete a1 = spot::ltl_to_tgba(f1);
    spot::tgba_bdd_concrete a2 = spot::ltl_to_tgba(f2);
    spot::ltl::destroy(f1);
    spot::ltl::destroy(f2);

#ifdef BDD_CONCRETE_PRODUCT
    spot::tgba_bdd_concrete p = spot::product(a1, a2);
#else
    spot::tgba_bdd_product p(a1, a2);
#endif

    spot::dotty_reachable(std::cout, p);
  }

  assert(spot::ltl::atomic_prop::instance_count() == 0);
  assert(spot::ltl::unop::instance_count() == 0);
  assert(spot::ltl::binop::instance_count() == 0);
  assert(spot::ltl::multop::instance_count() == 0);
  return exit_code;
}