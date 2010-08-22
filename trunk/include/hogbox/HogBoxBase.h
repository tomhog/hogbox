#pragma once

#include <assert.h>
#include <osg/Object>
#include <osg/Node>
#include <osg/Image>
#include <osg/Texture>
#include <osg/Uniform>


namespace hogbox
{

//
//All HogBox types inherit from osg Object so that we
//can use osg ref_ptr form auto memory management
//

/** META_Box macro define the standard clone, isSameKindAs, className
  * and accept methods used by osg Object types. This also extends those
  * features for any hogbox specific needs (which so far ar nill :) )
**/
#define META_Box(library,name) \
        virtual osg::Object* cloneType() const { return new name (); } \
        virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new name (*this,copyop); } \
        virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const name *>(obj)!=NULL; } \
        virtual const char* className() const { return #name; } \
		virtual const char* libraryName() const { return #library; } \
		static const std::string xmlClassName(){ osg::ref_ptr<name> classType;return classType->className();}
        //virtual void accept(osg::NodeVisitor& nv) { if (nv.validNodeMask(*this)) { nv.pushOntoNodePath(this); nv.apply(*this); nv.popFromNodePath(); } } \

//convinience type defs for osg::ref_ptrs of basic types
typedef osg::ref_ptr<osg::Object> ObjectPtr;
typedef std::vector<ObjectPtr> ObjectPtrVector;
typedef osg::ref_ptr<osg::Node> NodePtr;
typedef std::vector<NodePtr> NodePtrVector;
typedef osg::ref_ptr<osg::Image> ImagePtr;
typedef std::vector<ImagePtr> ImagePtrVector;
typedef osg::ref_ptr<osg::Texture> TexturePtr;
typedef std::vector<TexturePtr> TexturePtrVector;
typedef osg::ref_ptr<osg::Uniform> UniformPtr;
typedef std::vector<UniformPtr> UniformPtrVector;
typedef osg::ref_ptr<osg::Shader> ShaderPtr;
typedef std::vector<ShaderPtr> ShaderPtrVector;

}; //end hogbox namespace