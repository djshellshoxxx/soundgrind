#include "soundgrind/engine.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
  using namespace soundgrind;
  CampaignConfig c;
  assert(planned_case_count(c,4)==1);
  c.unique_states=30;c.rounds_per_state=5;
  assert(planned_case_count(c,7)==150);
  assert(profile_modules("level1").size()==5);
  assert(profile_modules("level2").size()>profile_modules("level1").size());
  c.unique_states=1;c.rounds_per_state=1;c.dwell_ms=1;validate_config(c);
  bool rejected=false;try{c.block_size=0;validate_config(c);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
  rejected=false;try{c.block_size=64;c.modules={"missing.module"};validate_config(c);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
  rejected=false;try{profile_modules("unknown");}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
  std::cout<<"engine contracts passed\n";
}
