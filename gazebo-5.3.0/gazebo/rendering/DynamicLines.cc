/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
/* Desc: Dynamic line generator
 * Author: Nate Koenig
 * Date: 28 June 2007
 */

#include <math.h>

#include <cmath>
#include <sstream>
#include "gazebo/rendering/ogre_gazebo.h"

#include "gazebo/common/Exception.hh"
#include "gazebo/rendering/DynamicLines.hh"

using namespace gazebo;
using namespace rendering;

enum {POSITION_BINDING, TEXCOORD_BINDING};

/////////////////////////////////////////////////
DynamicLines::DynamicLines(RenderOpType opType)
{
  this->Init(opType, false);
  this->setCastShadows(false);
  this->dirty = true;
}

/////////////////////////////////////////////////
DynamicLines::~DynamicLines()
{
}

/////////////////////////////////////////////////
std::string DynamicLines::GetMovableType()
{
  return "gazebo::dynamiclines";
}

/////////////////////////////////////////////////
const Ogre::String &DynamicLines::getMovableType() const
{
  static Ogre::String moveType = DynamicLines::GetMovableType();
  return moveType;
}

/////////////////////////////////////////////////
void DynamicLines::AddPoint(const math::Vector3 &_pt,
                            const common::Color &_color)
{
  this->points.push_back(_pt);
  this->colors.push_back(_color);
  this->dirty = true;
}

/////////////////////////////////////////////////
void DynamicLines::AddPoint(double _x, double _y, double _z,
                            const common::Color &_color)
{
  this->AddPoint(math::Vector3(_x, _y, _z), _color);
}

/////////////////////////////////////////////////
void DynamicLines::SetPoint(unsigned int index, const math::Vector3 &value)
{
  if (index >= this->points.size())
  {
    std::ostringstream stream;
    stream << "Point index[" << index << "] is out of bounds[0-"
           << this->points.size()-1 << "]";
    gzthrow(stream.str());
  }

  this->points[index] = value;

  this->dirty = true;
}

/////////////////////////////////////////////////
void DynamicLines::SetColor(unsigned int _index, const common::Color &_color)
{
  this->colors[_index] = _color;
  this->dirty = true;
}

/////////////////////////////////////////////////
const math::Vector3& DynamicLines::GetPoint(unsigned int index) const
{
  if (index >= this->points.size())
  {
    gzthrow("Point index is out of bounds");
  }

  return this->points[index];
}

/////////////////////////////////////////////////
unsigned int DynamicLines::GetPointCount() const
{
  return this->points.size();
}

/////////////////////////////////////////////////
void DynamicLines::Clear()
{
  this->points.clear();
  this->dirty = true;
}

/////////////////////////////////////////////////
void DynamicLines::Update()
{
  if (this->dirty && this->points.size() > 1)
    this->FillHardwareBuffers();
}

/////////////////////////////////////////////////
void DynamicLines::CreateVertexDeclaration()
{
  Ogre::VertexDeclaration *decl =
    this->mRenderOp.vertexData->vertexDeclaration;

  decl->addElement(POSITION_BINDING, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
  decl->addElement(1, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
}

/////////////////////////////////////////////////
void DynamicLines::FillHardwareBuffers()
{
  int size = this->points.size();
  this->PrepareHardwareBuffers(size, 0);

  if (!size)
  {
    this->mBox.setExtents(Ogre::Vector3::ZERO, Ogre::Vector3::ZERO);
    this->dirty = false;
  }

  Ogre::HardwareVertexBufferSharedPtr vbuf =
    this->mRenderOp.vertexData->vertexBufferBinding->getBuffer(0);

  Ogre::Real *prPos =
    static_cast<Ogre::Real*>(vbuf->lock(Ogre::HardwareBuffer::HBL_NORMAL));
  {
    for (int i = 0; i < size; i++)
    {
      *prPos++ = this->points[i].x;
      *prPos++ = this->points[i].y;
      *prPos++ = this->points[i].z;

      this->mBox.merge(Ogre::Vector3(this->points[i].x,
                                     this->points[i].y, this->points[i].z));
    }
  }
  vbuf->unlock();

  // Update the colors
  Ogre::HardwareVertexBufferSharedPtr cbuf =
    this->mRenderOp.vertexData->vertexBufferBinding->getBuffer(1);

  Ogre::RGBA *colorArrayBuffer =
        static_cast<Ogre::RGBA*>(cbuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
  Ogre::RenderSystem *renderSystemForVertex =
        Ogre::Root::getSingleton().getRenderSystem();
  for (int i = 0; i < size; ++i)
  {
    Ogre::ColourValue color = Conversions::Convert(this->colors[i]);
    renderSystemForVertex->convertColourValue(color, &colorArrayBuffer[i]);
  }
  cbuf->unlock();

  // need to update after mBox change, otherwise the lines goes in and out
  // of scope based on old mBox
  this->getParentSceneNode()->needUpdate();

  this->dirty = false;
}