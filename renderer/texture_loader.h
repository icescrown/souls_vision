//
// Direct2D 纹理加载器 - 用于加载PNG图片资源
//

#ifndef SOULS_VISION_TEXTURE_LOADER_H
#define SOULS_VISION_TEXTURE_LOADER_H

#include <windows.h>
#include <d2d1.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace souls_vision {

// D2D纹理信息结构（避免与shared_types.h中的TextureInfo冲突）
struct D2DTextureInfo {
    ID2D1Bitmap* bitmap;
    int width;
    int height;
    std::string name;
    
    D2DTextureInfo() : bitmap(nullptr), width(0), height(0) {}
};

// 纹理加载器类
class TextureLoader {
public:
    TextureLoader();
    ~TextureLoader();
    
    // 初始化，传入渲染目标
    bool Initialize(ID2D1RenderTarget* renderTarget);
    
    // 从文件加载纹理
    bool LoadTexture(const std::string& filename, const std::string& name);
    
    // 获取已加载的纹理
    D2DTextureInfo* GetTexture(const std::string& name);
    
    // 加载所有默认资源
    bool LoadAllResources();
    
    // 释放所有纹理
    void ReleaseAll();
    
private:
    ID2D1RenderTarget* renderTarget_;
    std::unordered_map<std::string, D2DTextureInfo> textures_;
    std::wstring resourcePath_;
    
    // 使用 WIC 加载 PNG 文件
    ID2D1Bitmap* LoadBitmapFromFile(const wchar_t* filename);
};

} // namespace souls_vision

#endif // SOULS_VISION_TEXTURE_LOADER_H
