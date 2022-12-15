#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/epsilon.hpp"

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( WIN32 ) || defined( _WINDOWS )
#    define DLL_EXPORT __declspec(dllexport)
#    define DLL_IMPORT __declspec(dllimport)
#    if defined POISSON_EXPORTS
#        define _poissonExport DLL_EXPORT
#    else
#        define _poissonExport DLL_IMPORT
#    endif
#else
#    define DLL_EXPORT
#    define DLL_IMPORT
#    define _poissonExport
#endif

namespace poisson
{
    static const float EPSILON = static_cast<float>(1.e-6);
    class _poissonExport PoissonDiskMultiSampler
    {
    public:

        struct Circle : public glm::vec2
        {
            Circle(float x, float y, float r) : glm::vec2(x, y), radius(r), active(true){}

            bool operator == (const Circle& rhs) const
            {
                return (glm::epsilonEqual(rhs.x, x, EPSILON) &&
                    glm::epsilonEqual(rhs.y, y, EPSILON) &&
                    glm::epsilonEqual(rhs.radius, radius, EPSILON));
            }

            float  radius;
            bool   active;
        };

        struct RealFunction2D
        {
            virtual bool placeObject(int layerIndex, float x, float y) = 0;
        };

        struct Vector2DInt
        {
            Vector2DInt() : x(0), y(0){}
            Vector2DInt(const Vector2DInt& rhs) : x(rhs.x), y(rhs.y){}       
            Vector2DInt(uint32_t x_, uint32_t y_) : x(x_), y(y_){}       
            uint32_t x;
            uint32_t y;
        };

        typedef std::vector<Circle> PointList;
        typedef std::vector<PointList> PointListArray;

        PoissonDiskMultiSampler(float x0,
            float y0,
            float x1,
            float y1,
            std::vector<float> minDist_,
            std::vector<float> minRadii_,
            std::vector<float> radii_,
            int maxPoints_,
            RealFunction2D& distribution_,
            bool multiLayer_,
            int pointsToGenerate_ = DEFAULT_POINTS_TO_GENERATE);

        typedef std::vector<PointListArray> Grid;
        typedef std::vector<Grid>           Grids;

        void sample(PointListArray& pointListArray, Grids precalculatedLayerGrids = Grids(), int seed = 1);

        Grids grids;

    private:
        int maxPoints;
        static int DEFAULT_POINTS_TO_GENERATE;
        int pointsToGenerate;
        glm::vec2 p0, p1;
        glm::vec2 dimensions;
        std::vector<float> cellSize;
        std::vector<float> minDist;
        std::vector<float> radii;
        std::vector<float> minRadii;
        std::vector<int> gridWidth, gridHeight;
        int layerCount;
        bool multiLayer;
        RealFunction2D& distribution;

        void initGrid(Grid& grid, int rows, int cols);

        static float randomFloat();
        static uint32_t randomInt(uint32_t max);
        bool pointCollisionDetected(Circle q, int layerIndex, Grids& grids);
        bool addNextPoint(Grid& grid, PointList& activeList, PointList& pointList, Circle point, int layerIndex);
        static Circle generateAround(glm::vec2 centre, float minDist, float minRadius, float radius, float distanceScale, float angleScale, float radiusScale);  
        void addFirstPoint(Grid& grid, PointList& activeList, PointList& pointList, int layerIndex);
        static Vector2DInt pointToInt(glm::vec2 point, glm::vec2 origin, float cellSize);
    };
}
