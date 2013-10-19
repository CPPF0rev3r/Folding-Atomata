
/******************************************************************************\
                     This file is part of Folding Atomata,
          a program that displays 3D views of Folding@home proteins.

                      Copyright (c) 2013, Jesse Victors

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see http://www.gnu.org/licenses/

                For information regarding this software email:
                                Jesse Victors
                         jvictors@jessevictors.com
\******************************************************************************/

#define _GLIBCXX_USE_NANOSLEEP

#include "Viewer.hpp"
#include "FAHClientIO.hpp"
#include "Sockets/SocketException.hpp"
#include "PyON/TrajectoryParser.hpp"
#include "Modeling/DataBuffers/SampledBuffers/Image.hpp"
#include "Modeling/DataBuffers/SampledBuffers/TexturedCube.hpp"
#include "Options.hpp"
#include <thread>
#include <algorithm>
#include <iostream>

/*
    1) return a vector of all trajectories for all slots
    2) display only slot 0
    3) display only snapshot 0
        draw atoms
        draw bonds
    4) then draw multiple snapshots (0, 1, 2, 3, ..., 0, 1, 2, 3, ...)

*/


Viewer::Viewer() :
    scene_(std::make_shared<Scene>(createCamera())),
    user_(std::make_shared<User>(scene_)),
    timeSpentRendering_(0), frameCount_(0), needsRerendering_(true)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    addModels();
    user_->grabPointer();
    reportFPS();
}



void Viewer::reportFPS()
{
    std::thread fpsReporter([&]()
    {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            float msPerFrame = timeSpentRendering_ / frameCount_;
            std::cout << frameCount_ << " FPS, spent " <<
                timeSpentRendering_ << " ms rendering, avg " <<
                msPerFrame << " ms/frame" << std::endl;
            frameCount_ = 0;
            timeSpentRendering_ = 0;
        }
    });

    fpsReporter.detach();
}



void Viewer::addModels()
{
    addSlotViewers();

    if (!Options::getInstance().skyboxDisabled())
        addSkybox();
}



void Viewer::addSkybox()
{
    std::cout << "Creating skybox..." << std::endl;

    auto imageA = std::make_shared<Image>(Options::getInstance().getPathToImageA());
    auto imageB = std::make_shared<Image>(Options::getInstance().getPathToImageB());
    auto imageC = std::make_shared<Image>(Options::getInstance().getPathToImageC());

    BufferList list = { std::make_shared<TexturedCube>(imageA, imageA,
        imageB, imageB, imageC, imageC) };
    auto matrix = glm::scale(glm::mat4(), glm::vec3(100));
    auto model = std::make_shared<InstancedModel>(getSkyboxMesh(), matrix, list);
    scene_->addModel(model); //add to Scene and save

    std::cout << "... done creating skybox." << std::endl;
}



void Viewer::addSlotViewers()
{
    std::vector<TrajectoryPtr> trajectories = getTrajectories();

    auto slot0Viewer = std::make_shared<SlotViewer>(trajectories[0], scene_);
    slotViewers_.push_back(slot0Viewer);
}



std::vector<TrajectoryPtr> Viewer::getTrajectories()
{
    std::vector<TrajectoryPtr> trajectories;

    try
    {
        auto socket = std::make_shared<ClientSocket>(
            Options::getInstance().getHost(),
            Options::getInstance().getPort()
        );

        FAHClientIO io(socket);
        trajectories = io.getTrajectories();

        if (trajectories.empty())
            std::cerr << "Not enough slots to work with. " <<
                "Using demo protein." << std::endl;
    }
    catch (SocketException&)
    {
        std::cerr << "Error connection to FAHClient (SocketException). " <<
                "Using demo protein." << std::endl;
    }

    if (trajectories.empty())
    {
        const std::string FILENAME = "/usr/share/FoldingAtomata/demoProtein";
        std::ifstream fin(FILENAME, std::ios::in | std::ios::binary);
        if (!fin.is_open())
            throw std::runtime_error("Unable to demo protein!");

        std::string proteinStr;
        fin.seekg(0, std::ios::end);
        proteinStr.resize((unsigned long)fin.tellg()); //allocate enough space
        fin.seekg(0, std::ios::beg);
        fin.read(&proteinStr[0], (long)proteinStr.size()); //read entire file
        fin.close();

        TrajectoryParser parser(proteinStr, false);
        trajectories.push_back(parser.parse());
    }

    return trajectories;
}



std::shared_ptr<Mesh> Viewer::getSkyboxMesh()
{
    static std::shared_ptr<Mesh> mesh = nullptr;

    if (mesh)
        return mesh;

    const std::vector<glm::vec3> VERTICES = {
        // front
        glm::vec3(-1.0, -1.0,  1.0),
        glm::vec3( 1.0, -1.0,  1.0),
        glm::vec3( 1.0,  1.0,  1.0),
        glm::vec3(-1.0,  1.0,  1.0),
        // top
        glm::vec3(-1.0,  1.0,  1.0),
        glm::vec3( 1.0,  1.0,  1.0),
        glm::vec3( 1.0,  1.0, -1.0),
        glm::vec3(-1.0,  1.0, -1.0),
        // back
        glm::vec3( 1.0, -1.0, -1.0),
        glm::vec3(-1.0, -1.0, -1.0),
        glm::vec3(-1.0,  1.0, -1.0),
        glm::vec3( 1.0,  1.0, -1.0),
        // bottom
        glm::vec3(-1.0, -1.0, -1.0),
        glm::vec3( 1.0, -1.0, -1.0),
        glm::vec3( 1.0, -1.0,  1.0),
        glm::vec3(-1.0, -1.0,  1.0),
        // left
        glm::vec3(-1.0, -1.0, -1.0),
        glm::vec3(-1.0, -1.0,  1.0),
        glm::vec3(-1.0,  1.0,  1.0),
        glm::vec3(-1.0,  1.0, -1.0),
        // right
        glm::vec3( 1.0, -1.0,  1.0),
        glm::vec3( 1.0, -1.0, -1.0),
        glm::vec3( 1.0,  1.0, -1.0),
        glm::vec3( 1.0,  1.0,  1.0)
    };

    //visible from the inside only, so faces in
    const std::vector<GLuint> INDICES = {
        // front
        2, 1, 0,
        0, 3, 2,
        // top
        6, 5, 4,
        4, 7, 6,
        // back
        10, 9, 8,
        8, 11, 10,
        // bottom
        14, 13, 12,
        12, 15, 14,
        // left
        18, 17, 16,
        16, 19, 18,
        // right
        22, 21, 20,
        20, 23, 22
    };

    auto vBuffer = std::make_shared<VertexBuffer>(VERTICES);
    auto iBuffer = std::make_shared<IndexBuffer>(INDICES, GL_TRIANGLES);
    mesh = std::make_shared<Mesh>(vBuffer, iBuffer, GL_TRIANGLES);
    return mesh;
}


std::shared_ptr<Camera> Viewer::createCamera()
{
    auto camera = std::make_shared<Camera>();
    camera->setPosition(glm::vec3(0, -50, 0));

    camera->lookAt(
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1)
    );

    return camera;
}



void Viewer::update(int deltaTime)
{
    user_->update(deltaTime);
    //note: tests of whether or not the player is moving is done in render()
}



void Viewer::animate(int deltaTime)
{
    bool animationHappened = false;
    for (auto viewer : slotViewers_)
        if (viewer->animate(deltaTime)) //test if animation happened
            animationHappened = true;

    if (animationHappened)
        needsRerendering_ = true; //the atoms moved, so redraw the scene
}



void Viewer::render()
{
    if (!needsRerendering_ && !user_->isMoving())
        return;
    needsRerendering_ = false; //it was true, so reset it and then render

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    timeSpentRendering_ += scene_->render();
    frameCount_++;

    glutSwapBuffers();
}



void Viewer::handleWindowReshape(int newWidth, int newHeight)
{
    scene_->getCamera()->setAspectRatio(newWidth / (float)newHeight);
    user_->setWindowOffset(glutGet(GLUT_WINDOW_X), glutGet(GLUT_WINDOW_Y));
    needsRerendering_ = true; //need to redraw after window update

    std::cout << "Windows updated to " << newWidth << " by " << newHeight <<
        ", a ratio of " << (newWidth / (float)newHeight) << std::endl;
}



std::shared_ptr<User> Viewer::getUser()
{
    return user_;
}



Viewer* Viewer::singleton_ = 0;

Viewer& Viewer::getInstance()
{
    try
    {
        if (singleton_)
            return *singleton_;

        std::cout << "Creating Viewer..." << std::endl;
        singleton_ = new Viewer();
        std::cout << "... done creating Viewer." << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << std::endl;
        std::cerr << "Caught " << typeid(e).name() <<
            " during Viewer initiation: " << e.what() << std::endl;
        glutDestroyWindow(glutGetWindow());
    }

    return *singleton_;
}




/* num-slots
std::vector<glm::vec2> getOffsetVectors(int numberOfSlots)
{
    switch (numberOfSlots)
    {
        case 1 :
            return {
                glm::vec2(0, 0)
            };

        case 2 :
            return {
                glm::vec2(0, 1),
                glm::vec2(0, -1)
            }

        case 3 :
            return {
                glm::vec2(0, 1),
                glm::vec2(1, -0.7),
                glm::vec2(-1, -0.7)
            }

        case 4 :
            return {
                glm::vec2(-1, 1),
                glm::vec2(1, -1),
                glm::vec2(1, 1),
                glm::vec2(-1, -1)
            }

        case 5 :
            return {
                glm::vec2(-1, 1),
                glm::vec2(1, -1),
                glm::vec2(1, 1),
                glm::vec2(-1, -1),
                glm::vec2(0, 0)
            }

        default :
            throw std::runtime_error("Too many slots");
    }
}

static std::vector<SlotViewer> viewAllSlots(const std::string& host, int port);
*/


/*
    http://stackoverflow.com/questions/5988686/creating-a-3d-sphere-in-opengl-using-visual-c

    protected:
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> texcoords;
    std::vector<GLushort> indices;

public:
    SolidSphere(float radius, unsigned int rings, unsigned int sectors)
    {
        float const R = 1./(float)(rings-1);
        float const S = 1./(float)(sectors-1);
        int r, s;

        vertices.resize(rings * sectors * 3);
        normals.resize(rings * sectors * 3);
        texcoords.resize(rings * sectors * 2);
        std::vector<GLfloat>::iterator v = vertices.begin();
        std::vector<GLfloat>::iterator n = normals.begin();
        std::vector<GLfloat>::iterator t = texcoords.begin();
        for(r = 0; r < rings; r++) for(s = 0; s < sectors; s++) {
                float const y = sin( -M_PI_2 + M_PI * r * R );
                float const x = cos(2*M_PI * s * S) * sin( M_PI * r * R );
                float const z = sin(2*M_PI * s * S) * sin( M_PI * r * R );

                *t++ = s*S;
                *t++ = r*R;

                *v++ = x * radius;
                *v++ = y * radius;
                *v++ = z * radius;

                *n++ = x;
                *n++ = y;
                *n++ = z;
        }

        indices.resize(rings * sectors * 4);
        std::vector<GLushort>::iterator i = indices.begin();
        for(r = 0; r < rings-1; r++) for(s = 0; s < sectors-1; s++) {
                *i++ = r * sectors + s;
                *i++ = r * sectors + (s+1);
                *i++ = (r+1) * sectors + (s+1);
                *i++ = (r+1) * sectors + s;
        }
    }
    */

//slot-info, in between "\"id\": \"" and "\",\n"
    //1) check for heartbeat, throw error if no connection
    //2) get topology. This data should be accessible as soon it's available.
    //3) get checkpoints. Can view more and more of them as they come in.
    /*
    try
    {
        SlotViewer viewer(Connection("localhost", 36330), 0);

        while (true)
        {
            std::chrono::milliseconds duration(2000);
            std::this_thread::sleep_for(duration);
            std::cout << "Main looping..." << std::endl;
        }

    }
    catch (SocketException& e)
    {
        std::cout << "Exception was caught: " << e.description() << "\n";
    }
*/
