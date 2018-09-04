#include "index.h"

namespace doc_index{
DEFINE_string(dict_path,
    "/home/syf/third_part/data/jieba_dict/jieba.dict.utf8",
    "词典路径");
DEFINE_string(hmm_path,
    "/home/syf/third_part/data/jieba_dict/hmm_model.utf8",
    "hmm 词典路径");
DEFINE_string(user_dict_path,
    "/home/syf/third_part/data/jieba_dict/user.dict.utf8",
    "用户词典路径");
DEFINE_string(idf_path,
    "/home/syf/third_part/data/jieba_dict/idf.utf8",
    "idf 词典路径");
DEFINE_string(stop_word_path,
    "/home/syf/third_part/data/jieba_dict/stop_words.utf8",
    "暂停词词典路径");

Index* Index::_instance = NULL;

Index::Index() 
    :_jieba(FLAGS_dict_path,FLAGS_hmm_path,FLAGS_user_dict_path,FLAGS_idf_path,FLAGS_stop_word_path){
        CHECK(_stop_word_dict.Load(FLAGS_stop_word_path));
    }

const DocInfo* Index::GetDocInfo(uint64_t doc_id) const{
    if(doc_id >= _forward_index.size()){
        return NULL;
    }
    return &_forward_index[doc_id];
}

const InvertedList* Index::GetInvertedList(const std::string& key) const{
    auto it = _inverted_index.find(key);
    if(it == _inverted_index.end()){
        return NULL;
    }
    return &(it->second);
}

bool Index::Build(const std::string& input_path){
    //按行读取文件
    std::fstream file(input_path.c_str());
    CHECK(file.is_open()) << "input_path: " << input_path;
    std::string line;
    while(std::getline(file,line)){
        //将这一行制作为DocInfo
        const DocInfo* doc_info = BuildForward(line);
        CHECK(doc_info != NULL) << "doc_info is nullptr!!";
        BuildInverted(*doc_info);
    }
    //处理完文件后对倒排拉链进行排序
    SortInvertedList();
    file.close();
    LOG(INFO) << "Index Build Done!!!";
    return true;
}

const DocInfo* Index::BuildForward(const std::string& line){
    std::vector<std::string> tokens;
    //先对line进行切分
    //TODO   标记,地址还是引用修改,后续再说
    common::StringUtil::Split(line,&tokens,"\3");
    if(tokens.size() < 3){
        LOG(FATAL) << "Split line failed,ckenk your file tokens.size(): " << tokens.size();
        return NULL;
    }
    //构造DocInfo结构保存切分信息
    DocInfo doc_info;
    doc_info.set_docid(_forward_index.size());  //正排索引的个数作为docid
    doc_info.set_title(tokens[1]);
    doc_info.set_content(tokens[2]);
    doc_info.set_jump_url(tokens[0]);
    doc_info.set_show_url(tokens[0]);  //先暂时设为一样的
    //对标题和正文进行分词,将分词结果保存到DocInfo中
    SplitTitle(tokens[1],&doc_info);
    SplitContent(tokens[2],&doc_info);
    _forward_index.push_back(doc_info);
    //每次返回刚插入的doc_id
    return &_forward_index.back();
}

void Index::SplitTitle(const std::string& title,DocInfo* doc_info){
    std::vector<cppjieba::Word> words;
    //使用jieba进行分词,分词需要提供一个jieba自带的Word类型的vector
    _jieba.CutForSearch(title,words);
    //words中包含的就是分词结果Word,每一个Word中都有一个offset,表示起始位置的下标
    if(words.size() < 1){
        LOG(FATAL) << "SplitTitle failed!!! title: " << title;
        return;
    }
    for(size_t i = 0;i<words.size();++i){
        //protobuf的repeated字段使用add只是向其中添加一个内容,并没有赋值
        auto* token = doc_info->add_title_token();
        //添加后记得手动赋值
        token->set_beg(words[i].offset);
        if(i + 1 < words.size()){
            token->set_end(words[i+1].offset);
        }else{
            //不是words的size,是title的size
            /* token->set_end(words.size()); */
            token->set_end(title.size());
        }
    }
    return;
}

void Index::SplitContent(const std::string& content,DocInfo* doc_info){
    std::vector<cppjieba::Word> words;
    //具体方法和切分title基本一致
    _jieba.CutForSearch(content,words);
    if(words.size() < 1){
        LOG(FATAL) << "SplitContent failed!!!";
        return;
    }
    for(size_t i = 0;i<words.size();++i){
        auto* tokens = doc_info->add_content_token();
        //记得赋值
        tokens->set_beg(words[i].offset);
        if(i + 1 < words.size()){
            tokens->set_end(words[i+1].offset);
        }else{
            //不是words的size,而是正文的长度
            /* tokens->set_end(words.size()); */
            tokens->set_end(content.size());
        }
    }
    return;
}

void Index::BuildInverted(const DocInfo& doc_info){
    WordCnt word_cnt_map;
    //统计title中每个词出现的个数
    for(int i = 0;i < doc_info.title_token_size();++i){
        const auto& token = doc_info.title_token(i);
        std::string word = doc_info.title().substr(token.beg(),token.end()-token.beg());
        //这里要做模糊处理,将HELLO 和 hello等等看作是同样的词
        boost::to_lower(word);  //万幸有boost这种东西...
        //去掉暂停词
        if(_stop_word_dict.Find(word)){
            //直接跳过
            continue;
        }
        ++word_cnt_map[word].title_cnt;
    }
    //统计content中每个词出现的个数
    for(int i = 0;i<doc_info.content_token_size();++i){
        const auto& token = doc_info.content_token(i);
        std::string word = doc_info.content().substr(token.beg(),token.end()-token.beg());
        //大小写处理
        boost::to_lower(word);
        //去暂停词
        if(_stop_word_dict.Find(word)){
            continue;
        }
        ++word_cnt_map[word].content_cnt;
        if(word_cnt_map[word].content_cnt == 1){
            //第一次出现
            word_cnt_map[word].first_pos = token.beg();
        }
    }
    //更新倒排索引,遍历刚得到的map,用key去倒排索引中查找,如果不存在,就新增一项
    //如果已经存在,根据当前构造好的Weight结构添加到倒排拉链
    for(const auto& word_pair : word_cnt_map){
        Weight weight;
        weight.set_doc_id(doc_info.docid());
        weight.set_weight(CalWeight(word_pair.second.title_cnt,word_pair.second.content_cnt));
        weight.set_first_pos(word_pair.second.first_pos);
        //现获取当前词的倒排拉链
        InvertedList& inverted_list = _inverted_index[word_pair.first];
        inverted_list.push_back(weight);
    }
    return;
}

int Index::CalWeight(int title_cnt,int content_cnt){
    //简单直接的方式,粗略的认为正文中的10次抵得上标题中的一次
    return 10 * title_cnt + content_cnt;
}

void Index::SortInvertedList(){
    //将倒排拉链按照Weight降序排列
    for(auto& inverted_pair : _inverted_index){
        InvertedList& inverted_list = inverted_pair.second;
        //这里的排序可以写一个仿函数
        //目前先这样实现
        std::sort(inverted_list.beg(),inverted_list.end(),CmpWeight);
    }
}

bool Index::CmpWeight(const Weight& w1,const Weight& w2){
    return w1.weight() > w2.weight();
}

//降内存中的索引保存到磁盘上
bool Index::Save(const std::string& output_path){
    LOG(INFO) << "Index Save";
    //现将内存中的索引结构序列化为字符串
    std::string proto_data;
    CHECK(ConvertToProto(&proto_data));
    //将序列化的到的字符串写到文件中
    CHECK(common::FileUtil::Write(output_path,proto_data));
    LOG(INFO) << "Index Build Done!!!";
    return true;
}

bool Index::ConvertToProto(std::string* proto_data){
    doc_index_proto::Index index;
    //将内存中的数据设置到index
    //设置正排
    for(const auto& doc_info : _forward_index){
        auto* proto_doc_info = index.add_forward_index();
        *proto_doc_info = doc_info;
    }
    //设置倒排
    for(const auto& inverted_pair: _inverted_index){
        auto* kwd_info = index.add_inverted_index();
        kwd_info->set_key(inverted_pair.first);
        for(const auto& weight : inverted_pair.second){
            auto* proto_weight = kwd_info->add_doc_list();
            *proto_weight = weight;
        }
    }
    index.SerializeToString(proto_data);
    return true;
}

// 把磁盘上的文件加载到内存的索引结构中
bool Index::Load(const std::string& index_path){
    LOG(INFO) << "Index Load";
    //从磁盘将索引文件读到内存中
    std::string proto_data;
    CHECK(common::FileUtil::Read(index_path,&proto_data));
    //反序列化,转成内存的索引结构
    CHECK(ConvertFromProto(proto_data));
    LOG(INFO) << "Index Load Done";
    return true;
}

bool Index::ConvertFromProto(const std::string& proto_data){
    //对索引文件进行反序列化
    doc_index_proto::Index index;
    index.ParseFromString(proto_data);
    //将正排索引放入内存
    for(int i = 0;i < index.inverted_index_size();++i){
        const auto& doc_info = index.forward_index(i);
        _forward_index.push_back(doc_info);
    }
    //将倒排索引放入内存中
    for(int i = 0;i < index.inverted_index_size();++i){
        const auto& kwd_info = index.inverted_index(i);
        InvertedList& inverted_list = _inverted_index[kwd_info.key()];
        for(int j = 0;j < kwd_info.weight_size();++j){
            const auto& weight = kwd_info.weight(j);
            inverted_list.push_back(weight);
        }
    }
    return true;
}

bool Index::Dump(const std::string& forward_dump_path,const std::string& inverted_dump_path){
    std::ofstream forward_dump_file(forward_dump_path.c_str());
    CHECK(forward_dump_file.is_open());
    for(const auto& doc_info : _forward_index){
        forward_dump_file << doc_info.Utf8DebugString() << 
            "==========================" << std::endl;
    }
    forward_dump_file.close();
    std::ofstream inverted_dump_file(inverted_dump_path.c_str());
    CHECK(inverted_dump_file.is_open());
    for(const auto& inverted_pair : _inverted_index){
        inverted_dump_file << inverted_pair.first << std::endl;
        for(const auto& weight : inverted_pair.second){
            inverted_dump_file << weight.Utf8DebugString();
        }
        inverted_dump_file << "===========================" << std::endl;
    }
    inverted_dump_file.close();
    return true;
}

bool Index::CutWordWithoutStopWord(const std::string& query,std::vector<std::string>* words){
    words->clear();
    //实现过滤暂停词
    std::vector<stf::string> tmp;
    for(std::string word : tmp){
        boost::to_lower(word);
        if(_stop_word_dict.Find(word)){
            continue;
        }
        words->push_back(word);
    }
    return true;
}

}  // end doc_index
