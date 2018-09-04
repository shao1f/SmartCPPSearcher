#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include <sys/time.h>

namespace common {

class StringUtil{
public:
    static void Split(const std::string& input,
                      std::vector<std::string>* output,
                      const std::string& split_char){
        //参数说明,1.要切分的字符串,2.切分结果存放,3.分隔符
        boost::split(*output,input,boost::is_any_of(split_char),boost::token_compress_off);
    }
};

class DictUtil{
public:
    //从文件加载数据到内存中
    bool Load(const std::string& file_path){
        std::ifstream file(file_path.c_str());
        if(!file.is_open()){
            return false;
        }
        std::string line;
        while(std::getline(file,line)){
            //暂停词是按行放置在文件中的
            _set.insert(line);
        }
        file.close();
        return true;
    }
    bool Find(const std::string &key) const{
        return _set.find(key) != _set.end();
    }

private:
    std::unordered_set<std::string> _set; //主要是存放词典相关,比如暂停词
};

//这里是文件相关的读写等操作,因为要大量用到,所以直接封装为一个类
class FileUtil{
public:
    static bool Read(const std::string& input_path,std::string* data){
        std::ifstream file(input_path.c_str());
        if(!file.is_open()){
            return false;
        }
        //文件长度的获取
        file.seekg(0,file.end);//现将文件指针挪到末尾
        int64_t length = file.tellg(); //int64_t是坑,问题在于只能加载2g左右的文件
        file.seekg(0,file.beg);
        //将指定长度的数据读取到内存中
        data->resize(length);
        //下面要注意,在C++11中data和c_str()已经一样了
        file.read(const_cast<char*>(data->data()),length);
        file.close();
        return true;
    }
    static bool Write(const std::string& output_path,const std::string& data){
        std::ofstream file(output_path.c_str());
        if(!file.is_open()){
            return false;
        }
        //用data更见名知意
        file.write(data.data(),data.size());
        file.close();
        return true;
    }
};

//构造时间戳相关的工具
class TimeUtil{
public:
    //秒级时间戳
    static int64_t TimeStamp(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec;
    }

    //毫秒级时间戳
    static int64_t TimeUtilMS(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }

    //微秒级时间戳
    static int64_t TimeStampUS(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec * 1e6 + tv.tv_usec;
    }
};

}  // end common
