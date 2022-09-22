#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

#include <sys/stat.h>
#include <dirent.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>
#include <ctime>
#include <cerrno>
#include <cstring>

//layer file extension is .lyr
//founder layer extension is .flyr

class LayerHeader{
    public:
        int mtime{0};
        int offset{0};
        int size{0};
        int writer_id{0};
        int layer_id{0};
    public:
        LayerHeader(){};
        int get_lh(std::string path);
        int set_lh(std::string path, int mtime, int offset, int size, int writer_id, int layer_id);
};

int LayerHeader::get_lh(std::string path){
    int rc{0};
    rc = getxattr(path.c_str(), "user.mtime", &mtime, sizeof(mtime));
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = getxattr(path.c_str(), "user.offset", &offset, sizeof(offset));
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = getxattr(path.c_str(), "user.size", &size, sizeof(size));
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = getxattr(path.c_str(), "user.writer_id", &writer_id, sizeof(writer_id));
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = getxattr(path.c_str(), "user.layer_id", &layer_id, sizeof(layer_id));
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    return 0;
}

int LayerHeader::set_lh(std::string path, int mtime, int offset, int size, int writer_id, int layer_id){
    int rc{0};

    rc = setxattr(path.c_str(), "user.mtime", &mtime, sizeof(mtime), 0);
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = setxattr(path.c_str(), "user.offset", &offset, sizeof(offset), 0);
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = setxattr(path.c_str(), "user.size", &size, sizeof(size), 0);
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = setxattr(path.c_str(), "user.writer_id", &writer_id, sizeof(writer_id), 0);
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = setxattr(path.c_str(), "user.layer_id", &layer_id, sizeof(layer_id), 0);
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    return 0;
}

class Layer{
    public:
        std::string layer_path;
        LayerHeader hdr;
    private:
        int init_hdr(int mtime, int offset, int size, int writer_id, int layer_id);
    public:
        Layer(){};
        int load(std::string layer_path_);
        int create(std::string path, const char* buffer, int offset, int size, int writer_id, int layer_id, int mtime);
        bool have_contents(int offset, int size);
        int read_internal(char* buffer, int offset, int size);
        int read_(char* buffer, int offset, int size);
};

int Layer::init_hdr(int mtime, int offset, int size, int writer_id, int layer_id){
    return hdr.set_lh(layer_path, mtime, offset, size, writer_id, layer_id);
}

int Layer::load(std::string layer_path_){
    layer_path = layer_path_;
    return hdr.get_lh(layer_path);
}

int Layer::create(std::string path, const char* buffer, int offset, int size, int writer_id, int layer_id, int mtime){
    int fd{0},rc{0};
    //{path}/{filename}%{writer_id}%{layer_id}.lyr
    layer_path = path + "/" + std::to_string(writer_id) + "%" + std::to_string(layer_id) + ".lyr";
    fd = creat(layer_path.c_str(), 0600);
    if(fd < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    rc = write(fd, buffer, size);
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        close(fd);
        return -errno;
    }
    close(fd);
    return init_hdr(mtime, offset, size, writer_id, layer_id);
}

bool Layer::have_contents(int offset, int size){
    //fully include
    if(offset >= hdr.offset && (offset + size) >= (hdr.offset + hdr.size)){
        return true;
    }
    //fully included
    if(offset <= hdr.offset && (offset + size) >= (hdr.offset + hdr.size)){
        return true;
    }
    //forward overlap
    if(offset > hdr.offset && (hdr.offset + hdr.size) > offset && (hdr.offset + hdr.size) < (offset + size)){
        return true;
    }
    //backward overlap
    if(offset < hdr.offset && (offset + size) > hdr.offset && (hdr.offset + hdr.size) >= (offset + size)){
        return true;
    }
    return false;
}

int Layer::read_internal(char* buffer, int offset, int size){
    int rc{0};
    int fd = open(layer_path.c_str(), O_RDONLY); 
    if(fd < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    lseek(fd, offset, SEEK_SET);
    rc = read(fd, buffer, size);
    if(rc < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        close(fd);
        return -errno;
    }
    close(fd);
    return rc;
}

int Layer::read_(char* buffer, int offset, int size){
    int rc{0};
    int buffer_offset{0};
    int layer_offset{0};
    int layer_size{0};
    //fully include
    if(offset >= hdr.offset && (offset + size) >= (hdr.offset + hdr.size)){
        buffer_offset = 0;
        layer_offset = offset - hdr.offset;
        layer_size = size;
        rc = read_internal(buffer + buffer_offset, layer_offset, layer_size);
    }
    //fully included
    if(offset <= hdr.offset && (offset + size) >= (hdr.offset + hdr.size)){
        buffer_offset = hdr.offset - offset;
        layer_offset = 0;
        layer_size = hdr.size;
        rc = read_internal(buffer + buffer_offset, layer_offset, layer_size);
    }
    //forward overlap
    if(offset > hdr.offset && (hdr.offset + hdr.size) > offset && (hdr.offset + hdr.size) < (offset + size)){
        buffer_offset = 0;
        layer_offset = offset - hdr.offset;
        layer_size = hdr.size - layer_offset;
        rc = read_internal(buffer + buffer_offset, layer_offset, layer_size);
    }
    //backward overlap
    if(offset < hdr.offset && (offset + size) > hdr.offset && (hdr.offset + hdr.size) >= (offset + size)){
        buffer_offset = hdr.offset - offset;
        layer_offset = 0;
        layer_size = (offset + size) - hdr.offset;
        rc = read_internal(buffer + buffer_offset, layer_offset, layer_size);
    }
    if(rc < 0){
        return rc;
    }else{
        return buffer_offset + layer_size;
    }
}

class LayerFile{
    private:
        std::string path;
    private:
        void sort_by_time(std::vector<Layer> &layers);
        std::string generate_layer_path(int writer_id);
        std::string get_flyr_path();
        int list_layer_file(std::vector<std::string> &layer_paths);
        int scan(std::vector<Layer>& layers);
    public:
        LayerFile();
        int load_(std::string path);
        int creat_(std::string path);
        int stat_(struct stat& stbuf);
        int read_(char* buffer, int offset, int size);
        int write_(const char* buffer, int offset, int size, int writer_id, int mtime);
        int relocate(std::string path);
};

void LayerFile::sort_by_time(std::vector<Layer>& layers){
    std::sort(layers.begin(), layers.end(), [](Layer& x, Layer&y){
                return y.hdr.mtime > x.hdr.mtime;
            });
}

std::string LayerFile::get_flyr_path(){
    std::string fname = std::filesystem::path(path).filename();
    return path + "/" + fname + ".flyr";
}

int LayerFile::list_layer_file(std::vector<std::string> &layer_paths){
    layer_paths.clear();
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    dirent* dp;
    while((dp = readdir(dir)) != nullptr){
        std::string name = dp->d_name;
        if(name.ends_with(".lyr")){
            layer_paths.push_back(path + "/" + name);
        }
    }
    closedir(dir);
    return 0;
}

int LayerFile::scan(std::vector<Layer>& layers){
    int rc{0};
    layers.clear();
    std::vector<std::string> di;
    rc = list_layer_file(di);
    if(rc < 0){ return rc; }
    for(auto& layer_path : di){
        Layer layer;
        rc = layer.load(layer_path);
        if(rc < 0){
            return rc;
        }else{
            layers.push_back(layer);
        }
    }
    sort_by_time(layers);
    return 0;
}

LayerFile::LayerFile(){
}

int LayerFile::creat_(std::string path_){
    path = path_;
    if(mkdir(path.c_str(), 0700) < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    std::string flyrpath = get_flyr_path();
    int fd = creat(flyrpath.c_str(), 0700);
    if(fd < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    close(fd);
    return 0;
}

int LayerFile::stat_(struct stat& stbuf){
    int rc;
    if(stat(get_flyr_path().c_str(), &stbuf) < 0){
        std::cout << strerror(errno) << " " << __FUNCTION__ << std::endl;
        return -errno;
    }
    
    std::vector<Layer> layers;
    rc = scan(layers);
    if(rc < 0){ return rc; }

    //mtime
    if(layers.size() > 0){
        auto& last_layer = layers.back();
        stbuf.st_mtime = last_layer.hdr.mtime;
    }

    //size
    for(auto& layer : layers){
        int layer_end_offset = layer.hdr.offset + layer.hdr.size;
        if(stbuf.st_size < layer_end_offset){
            stbuf.st_size = layer_end_offset;
        }
    }

    return 0;
}

int LayerFile::read_(char* buffer, int offset, int size){
    int rc{0};
    int read_size{0};
    //error check

    std::vector<Layer> layers;
    rc = scan(layers);
    if(rc < 0){ return rc; }

    for(auto& layer : layers){
        rc = layer.read_(buffer, offset, size);
        if(rc < 0){ return rc; }
        if(read_size < rc){
            read_size = rc;
        }
    }
    return read_size - offset;
}

int LayerFile::write_(const char* buffer, int offset, int size, int writer_id, int mtime){
    int rc{0};
    //error check

    std::vector<Layer> layers;
    rc = scan(layers);
    if(rc < 0){ return rc; }

    //generate layer path and get max size;
    int layer_id = 0;
    int max_size{0};
    for(auto& layer : layers){
        if(layer.hdr.writer_id == writer_id && layer.hdr.layer_id >= layer_id){
            layer_id = layer.hdr.layer_id + 1;
        }
        if(max_size < (layer.hdr.offset + layer.hdr.size)){
            max_size = layer.hdr.offset + layer.hdr.size;
        }
    }

    //adjust offset
    if(max_size < offset){
        offset = max_size;
    }

    //create layer
    Layer new_layer;
    rc = new_layer.create(path, buffer, offset, size, writer_id, layer_id, mtime);
    if(rc < 0){ return rc; }

    return 0;
}

struct io_set{
    const char* buffer;
    int offset;
    int size;
};

int main(){
    int writer_id = 1;
    int rc{0};

    system("rm -rf sample");

    LayerFile file;
    
    rc = file.creat_("sample");
    if(rc < 0){ return rc; }

    struct stat attr;
    rc = file.stat_(attr);
    if(rc < 0){ return rc; }
    std::cout << attr.st_size << " " << attr.st_mtime << std::endl;

    {
        io_set wbuf1 = {"hello", 0, 5};
        char rbuf[32];
        bzero(rbuf, 32);
        rc = file.write_(wbuf1.buffer, wbuf1.offset, wbuf1.size, writer_id, time(nullptr));
        if(rc < 0){ return rc; }
        rc = file.read_(rbuf, 0, 32);
        if(rc < 0){ return rc; }
        std::cout << rbuf << std::endl;
    }
    {
        io_set wbuf1 = {"world", 5, 5};
        char rbuf[32];
        bzero(rbuf, 32);
        rc = file.write_(wbuf1.buffer, wbuf1.offset, wbuf1.size, writer_id, time(nullptr));
        if(rc < 0){ return rc; }
        rc = file.read_(rbuf, 0, 32);
        if(rc < 0){ return rc; }
        std::cout << rbuf << std::endl;
    }
    {
        io_set wbuf1 = {"yuta", 20, 4};
        char rbuf[32];
        bzero(rbuf, 32);
        rc = file.write_(wbuf1.buffer, wbuf1.offset, wbuf1.size, writer_id, time(nullptr));
        if(rc < 0){ return rc; }
        rc = file.read_(rbuf, 0, 32);
        if(rc < 0){ return rc; }
        std::cout << rbuf << std::endl;
    }
    {
        //create old layer directly
        io_set wbuf1 = {"old", 0, 3};
        char rbuf[32];
        bzero(rbuf, 32);
        //old write at 3000 (epoch sec)
        rc = file.write_(wbuf1.buffer, wbuf1.offset, wbuf1.size, writer_id, 3000);
        if(rc < 0){ return rc; }
        rc = file.read_(rbuf, 0, 32);
        if(rc < 0){ return rc; }
        std::cout << rbuf << std::endl;
    }
}
