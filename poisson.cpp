#include "poisson.h"
#include "glm/gtc/constants.hpp"

#include <algorithm>
#ifdef DEBUG_POISSON
#include <iostream>
#endif

namespace poisson
{

    int PoissonDiskMultiSampler::DEFAULT_POINTS_TO_GENERATE = 30;

    void PoissonDiskMultiSampler::initGrid(Grid& grid, int rows, int cols)
    {
        for(int r = 0; r < rows; ++r)
        {
            PointListArray row;
            for(int c = 0; c < cols; ++c)
            {
                PointList points;
                row.push_back(points);
            }
            grid.push_back(row);
        }
    }

    void PoissonDiskMultiSampler::initPointListArray(PointListArray& pointList, int size)
    {
        pointList.reserve(size);
        for(int i = 0; i < size; ++i)
        {
            PointList points;
            // TODO - reserve space for points (row * cols)
            pointList.push_back(points);
        }
    }

    float PoissonDiskMultiSampler::randomFloat()
    {
        return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    }

    uint32_t PoissonDiskMultiSampler::randomInt(uint32_t max)
    {
        return (rand() % static_cast<int>(max));
    }

    PoissonDiskMultiSampler::PoissonDiskMultiSampler(float x0,
        float y0,
        float x1,
        float y1,
        std::vector<float> minDist_,
        std::vector<float> minRadii_,
        std::vector<float> radii_,
        int maxPoints_,
        RealFunction2D& distribution_,
        bool multiLayer_,
        int pointsToGenerate_) : distribution(distribution_)
    {
        layerCount = minDist_.size();

        p0 = glm::vec2(x0, y0);
        p1 = glm::vec2(x1, y1);
        dimensions = glm::vec2(x1 - x0, y1 - y0);

        minDist = minDist_;
        radii = radii_;
        minRadii = minRadii_;
        maxPoints = maxPoints_;
        distribution = distribution_;
        pointsToGenerate = pointsToGenerate_;
        multiLayer = multiLayer_;

        cellSize.resize(layerCount);
        gridWidth.resize(layerCount);
        gridHeight.resize(layerCount);

        for (int k = 0; k < layerCount; ++k)
        {
            cellSize[k]   = minDist[k] / sqrtf(2);
            gridWidth[k]  = (int) (dimensions.x / cellSize[k]) + 1;
            gridHeight[k] = (int) (dimensions.y / cellSize[k]) + 1;
        }
    }

    void PoissonDiskMultiSampler::sample(PointListArray& pointListArray)
    {
        // create point lists for layers        
        initPointListArray(pointListArray, layerCount);

        // create a list of grids
        std::vector<Grid> grids;
        grids.reserve(layerCount);

        // for each layer
        for (int k = 0; k < layerCount; ++k)
        {
            // create a list to hold the active points
            PointList activeList;

            // create a grid at the current layer
            Grid grid;
            initGrid(grid, gridHeight[k], gridWidth[k]);
            grids.push_back(grid);

            // chooses a random point in the grid
            // adds it to the grid point into the cell relating to the point location
            // adds the point to the active list
            addFirstPoint(grids[k], activeList, pointListArray[k], k);

            // while we have active points and haven't reached our point limit
#ifdef DEBUG_POISSON
            int c = 0;
#endif
            while(!activeList.empty() /*&& (pointListArray[k].size() < maxPoints)*/)
            {
                // choose a random value between 0 and activeList.size() - 1
                int listIndex = randomInt(activeList.size());

                // get active point at the index of the randomly chosen value
                // nb. for the first selected active point the active point is 0,0 (i think?)
                Circle point = activeList.at(listIndex);
                bool found = false;

                // for each point to generate
                for (int m = 0; m < pointsToGenerate; m++)
                {
                    // generate a random point
                    // initialise "point" with generated point
                    // add generated point to grid, activeList and pointList[k]
                    found |= addNextPoint(grids[k], activeList, pointListArray[k], point, k);
                }

                // if we succesfully added points
                if (!found)
                {
                    // remove activePoint from list
                    activeList.erase(activeList.begin() + listIndex);
                }
#ifdef DEBUG_POISSON
                c++;
#endif
            }
#ifdef DEBUG_POISSON
            std::cout << "Poisson iterations for layer: " << k << " = " << c << std::endl;
#endif
        }

        if (multiLayer)
        {
            // for each layer
            for (int k = (layerCount-2); k >= 0; k--)
            {
                // for each point in the point list for the current layer
                for (PointList::iterator point = pointListArray[k].begin(); point != pointListArray[k].end();)
                {
                    // check that the point does not collide with any other above
                    Circle pt = *point;
                    if (!distribution.placeObject(k, pt.x, pt.y) || checkPoint(pt, k, grids))
                    {
                        // remove from grid
                        Vector2DInt index(pointToInt(pt, p0, cellSize[k]));
                        PointList::iterator it = std::find(grids[k][index.x][index.y].begin(), grids[k][index.x][index.y].end(), (*point));
                        assert(it != grids[k][index.x][index.y].end());
                        grids[k][index.x][index.y][std::distance(grids[k][index.x][index.y].begin(), it)].active = false;

                        // remove from point list
                        point = pointListArray[k].erase(point);
                    }
                    else
                        point++;
                }
            }
        }
    }

    bool PoissonDiskMultiSampler::checkPoint(Circle q, int layerIndex, Grids& grids)
    {
        int k = 0;
        bool tooClose = false;

        // for each layer above the current layer index and while the point being tested is not too close
        for (k = layerIndex + 1; (k <= (layerCount-1)) && !tooClose; k++)
        {
            // get the 2d array of points for the layer above
            const Grid& grid = grids.at(k);

            // convert the current layer point q to an index into the grid for layer above (k). p0 represents a 2d vector that is the origin of all grids
            Vector2DInt qIndex = pointToInt(q, p0, cellSize[k]);

            // perform a point collision check
            int32_t lIdx  = std::max<int32_t>(0, qIndex.x - 2);             // LEFT
            int32_t rIdx  = std::min<int32_t>(gridWidth[k],  qIndex.x + 2); // RIGHT
            int32_t tIdx  = std::min<int32_t>(gridHeight[k], qIndex.y + 2); // TOP
            int32_t bIdx  = std::max<int32_t>(0, qIndex.y - 2);             // BOTTOM
            for (int i = lIdx; (i < rIdx) && !tooClose; ++i)
            {
                // same as above but for the height
                for (int j = bIdx; (j < tIdx) && !tooClose; ++j)
                {
                    // i == col index, j == row index, for each point in the list of points for the grid cell i,j
                    for (const Circle& gridPoint : grid[i][j])
                    {
                        if(!gridPoint.active)
                            continue;

                        // perform a distance check
                        float distance = glm::distance(static_cast<glm::vec2>(gridPoint), static_cast<glm::vec2>(q));
                        if((distance < (q.radius + gridPoint.radius)))
                        {
                            tooClose = true;
                            break;
                        }
                    }
                }
            }
        }
        return tooClose;
    }

    bool PoissonDiskMultiSampler::addNextPoint(Grid& grid, PointList& activeList, PointList& pointList, Circle point, int layerIndex)
    {
        bool found = false;

        // generate a random point within the bounds of the grid (p0,p1)
        float  r = radii[layerIndex];
        float fraction = 1.f;
        Circle q = generateAround(point,
            minDist[layerIndex],
            minRadii[layerIndex],
            radii[layerIndex],
            fraction,
            randomFloat(),
            randomFloat());

        // if the point is in bounds
        if (((q.x-r) > p0.x) && ((q.x+r) < p1.x) && ((q.y-r) > p0.y) && ((q.y+r) < p1.y))
        {
            // get the index of the point
            Vector2DInt qIndex = pointToInt(q, p0, cellSize[layerIndex]);

            // perform locality check
            bool tooClose = false;
            for (int i = std::max<int32_t>(0, qIndex.x - 2); (i < std::min<int32_t>(gridWidth[layerIndex], qIndex.x + 3)) && !tooClose; i++)
            {
                for (int j = std::max<int32_t>(0, qIndex.y - 2); (j < std::min<int32_t>(gridHeight[layerIndex], qIndex.y + 3)) && !tooClose; j++)
                {
                    for (Circle gridPoint : grid[i][j])
                    {
                        float distance = glm::distance(static_cast<glm::vec2>(gridPoint), static_cast<glm::vec2>(q));
                        if (distance < minDist[layerIndex] * fraction)
                        {
                            tooClose = true;
                            break;
                        }
                    }
                }
            }

            // add the point to the activelist, pointlist and grid if its not too close
            if (!tooClose)
            {
                found = true;
                activeList.push_back(q);
                pointList.push_back(q);
                grid[qIndex.x][qIndex.y].push_back(q);
            }
        }

        return found;
    }

    PoissonDiskMultiSampler::Circle PoissonDiskMultiSampler::generateAround(glm::vec2 centre, float minDist, float minRadius, float radius, float distanceScale, float angleScale, float radiusScale)
    {
        float r = (minDist + minDist * (distanceScale));
        float angle = 2.f * glm::pi<float>() * (angleScale);
        float newX = r * std::cos(angle);
        float newY = r * std::sin(angle);
        float newRadius = minRadius + radiusScale * (radius - minRadius);
        return Circle(centre.x + newX, centre.y + newY, newRadius);
    }

    void PoissonDiskMultiSampler::addFirstPoint(Grid& grid, PointList& activeList, PointList& pointList, int layerIndex)
    {
        float d = randomFloat();
        float xr = p0.x + radii[layerIndex] + (dimensions.x - (2.f*radii[layerIndex])) * (d);

        d = randomFloat();
        float yr = p0.y + radii[layerIndex] + (dimensions.y - (2.f*radii[layerIndex])) * (d);

        d = randomFloat();
        float rr = minRadii[layerIndex] + d * (radii[layerIndex] - minRadii[layerIndex]);
        Circle p(xr, yr, rr);
        Vector2DInt index(pointToInt(p, p0, cellSize[layerIndex]));

        grid[index.x][index.y].push_back(p);
        activeList.push_back(p);
        pointList.push_back(p);
    }

    PoissonDiskMultiSampler::Vector2DInt PoissonDiskMultiSampler::pointToInt(glm::vec2 point, glm::vec2 origin, float cellSize)
    {
        return Vector2DInt((int) ((point.x - origin.x) / cellSize), (int) ((point.y - origin.y) / cellSize));
    }

}

