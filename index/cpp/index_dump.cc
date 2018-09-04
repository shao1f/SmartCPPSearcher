#include <base/base.h>
#include "index.h"

DEFINE_string(index_path, "../data/output/index_file",
              "索引文件路径");
DEFINE_string(forward_dump_path,
              "../data/tmp/forward_dump_file",
              "正排索引反解路径");
DEFINE_string(inverted_dump_path,
              "../data/tmp/inverted_dump_file",
              "倒排索引反解路径");

int main(int argc, char* argv[]) {
  base::InitApp(argc, argv);
  doc_index::Index* index = doc_index::Index::Instance();
  CHECK(index->Load(fLS::FLAGS_index_path));
  CHECK(index->Dump(fLS::FLAGS_forward_dump_path,
        fLS::FLAGS_inverted_dump_path));
  return 0;
}
