#include "nodeserver.hpp"

using namespace node;

int main(int argc, char** argv) {
  // Expect only arg: --db_path=path/to/route_guide_db.json.
  RunServer();

  return 0;
}