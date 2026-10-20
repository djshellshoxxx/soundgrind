#include "soundgrind/clap_adapter.hpp"
#include <fstream>
#include <iostream>

int main(int argc,char** argv) {
  std::filesystem::path request,result;
  for(int i=1;i+1<argc;++i){std::string a=argv[i];if(a=="--request")request=argv[++i];else if(a=="--result")result=argv[++i];}
  if(request.empty()||result.empty()){std::cerr<<"usage: soundgrind_worker --request FILE --result FILE\n";return 2;}
  try {
    std::ifstream in(request);if(!in)throw std::runtime_error("cannot open worker request");auto req=nlohmann::json::parse(in);
    auto response=soundgrind::run_clap_request(req);std::ofstream out(result,std::ios::trunc);if(!out)throw std::runtime_error("cannot create worker response");out<<response.dump(2)<<'\n';out.flush();if(!out)throw std::runtime_error("failed to flush worker response");return 0;
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
