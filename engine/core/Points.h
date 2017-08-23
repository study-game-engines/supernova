
#ifndef POINTS_H
#define POINTS_H

#define S_POINTSIZE_PIXEL 0
#define S_POINTSIZE_WIDTH 1
#define S_POINTSIZE_HEIGHT 2

#include "ConcreteObject.h"
#include "render/ObjectRender.h"
#include <unordered_map>

namespace Supernova {

    class Points: public ConcreteObject {

    private:
        //std::vector<float> pointSizesScaled;
        //std::vector<float> rectsData;
        
        int texWidth;
        int texHeight;

        float pointScale;

        int pointSizeReference;
        
        bool useTextureRects;

        void updatePointScale();
        void fillPointsData();
        void deletePointsData();
        std::vector<int> findFramesByString(std::string id);

    protected:

        struct FramesData{
            std::string id;
            Rect rect;
        };

        struct Point{
            Vector3 position;
            Vector3 normal;
            Rect* textureRect;
            float size;
            Vector4 color;
        };

        ObjectRender* render;

        std::vector<Point> points;

        float* positionsData;
        float* normalsData;
        float* textureRectsData;
        float* sizesData;
        float* colorsData;

        bool sizeAttenuation;
        float pointScaleFactor;
        float minPointSize;
        float maxPointSize;

        std::vector<FramesData> framesRect;
        
        void updatePositions();
        void updateNormals();
        void updatePointColors();
        void updatePointSizes();
        void updateTextureRects();

    public:
        Points();
        virtual ~Points();

        void setSizeAttenuation(bool sizeAttenuation);
        void setPointScaleFactor(float pointScaleFactor);
        void setPointSizeReference(int pointSizeReference);
        void setMinPointSize(float minPointSize);
        void setMaxPointSize(float maxPointSize);

        void addPoint();
        void addPoint(Vector3 position);
        
        void clearPoints();

        void setPointPosition(int point, Vector3 position);
        void setPointPosition(int point, float x, float y, float z);
        void setPointSize(int point, float size);
        void setPointColor(int point, Vector4 color);
        void setPointColor(int point, float red, float green, float blue, float alpha);
        void setPointSprite(int point, int index);
        void setPointSprite(int point, std::string id);
        
        void addSpriteFrame(std::string id, float x, float y, float width, float height);
        void addSpriteFrame(float x, float y, float width, float height);
        void addSpriteFrame(Rect rect);
        void removeSpriteFrame(int index);
        void removeSpriteFrame(std::string id);
        
        virtual void updateVPMatrix(Matrix4* viewMatrix, Matrix4* projectionMatrix, Matrix4* viewProjectionMatrix, Vector3* cameraPosition);
        virtual void updateMatrix();
        
        virtual bool renderDraw();
        
        virtual bool load();
        virtual void destroy();
    };
    
}


#endif
