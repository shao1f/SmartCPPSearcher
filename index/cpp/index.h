#pragma once

#include <base/base.h>
#include <unordered_map>
#include <cppjieba/Jieba.hpp>
#include "index.pb.h"
#include "../../common/util.hpp"

//将需要用的一些结构重定义一下,方便使用
typedef doc_index_proto::DocInfo DocInfo;
typedef doc_index_proto::Weight Weight;
typedef std::vector<DocInfo> ForwardIndex;  //正排索引结构
typedef std::vector<Weight> InvertedList;  //倒排拉链
typedef std::unordered_map<std::string,InvertedList> InvertedIndex;  //倒排索引结构


namespace doc_index{

//分词结果次数的统计
struct WordCnt{
    int title_cnt;
    int content_cnt;
    //正文中第一次出现的位置,用来衡量倒排结果
    int first_pos;
    WordCnt()
        :title_cnt(0),content_cnt(0),first_pos(-1){}
    //初始化firstpos为-1为了方便判断是否盖茨在正文中出现过
};

typedef std::unordered_map<std::string,WordCnt> WordCntMap;

//将索引作为一个单例类,大文件的加载...
class Index{
public:
    static Index* Instance(){
        if(_instance == NULL){
            _instance = new Index();
        }
        return _instance;
    }
    Index();
    ~Index();

    //构建索引
    bool Build(const std::string& input_path);
    //保存索引,将内存中的结构通过protobuf序列化后存入磁盘
    bool Save(const std::string& output_path);
    //加载索引,只要是为了实现索引的反解,同时也是为了调试索引是否正确
    bool Load(const std::string& index_path);
    //索引反解,将序列化的索引进行反解
    bool Dump(const std::string& forward_dump_path,
              const std::string& inverted_dump_path);
    //正排的查询
    const DocInfo* GetDocInfo(uint64_t doc_id) const;
    //倒排的查询
    const InvertedList* GetInvertedList(const std::string& key) const;
    //查询词的分词,这里要注意去掉暂停词,否则会对结果产生很大的影响
    bool CutWordWithoutStopWord(const std::string& query,
                                std::vector<std::string>* words);
    static bool CmpWeight(const Weight w1,const Weight w2);

private:
    //这里的函数是为了实现索引模块构建而做的辅助函数,方便模块的划分和修改
    //尽量减少模块内部的耦合关系
    const DocInfo* BuildForward(const std::string& line);
    void BuildInverted(const DocInfo& doc_info);
    void SortInvertedList();
    void SplitTitle(const std::string& title,DocInfo* doc_info);
    void SplitContent(const std::string& content,DocInfo* doc_info);
    int CalWeight(int title_cnt,int content_cnt);
    bool ConvertToProto(std::string* proto_data);
    bool ConvertFromProto(const std::string& proto_data);
private:
    static Index* _instance;

    //正排索引
    ForwardIndex _forward_index;
    //倒排索引
    InvertedIndex _inverted_index;
    cppjieba::Jieba _jieba;  //结巴分词
    common::DictUtil _stop_word_dict;  //加载的暂停词词典

};

}  // end doc_index
