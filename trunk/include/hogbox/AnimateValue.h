#pragma once

#include <osg/MatrixTransform>
#include <osgAnimation/EaseMotion>
#include <hogbox/HogBoxBase.h>
#include <deque>

namespace hogbox 
{

	


//
//Animates a value based on a queue of keys, each key
//has a start value, end value, duration in secs and 
//an ease motion controler
//
template <class T>
class AnimateValue : public osg::Object
{
public:
	
	AnimateValue()
		: osg::Object(),
		m_keyFrameQueue(new KeyFrameQueue())
	{
	}
	AnimateValue(const T& value)
		: osg::Object(),
		m_value(value),
		m_start(value),
		m_keyFrameQueue(new KeyFrameQueue())
	{
	}
	
	/** Copy constructor using CopyOp to manage deep vs shallow copy.*/
	AnimateValue(const AnimateValue& value,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
		: osg::Object(value, copyop),
		m_value(value.m_value),
		m_start(value.m_start)
	{
	}
	
	META_Box(hogbox,AnimateValue);
	
	struct KeyFrame{
		T end;
		float duration;
		osg::ref_ptr<osgAnimation::Motion> motion;
	};
	
	//
	//KeyFrameQueue 
	//Wraps a dequeue of KeyFrames and handles selecting current frame as animation
	//updates. Callbacks can also be registered to receive notice when certain event occur
	//in this animation queue e.g. when the end of the queue is reached.
	//
	class KeyFrameQueue : public osg::Object 
	{
	public:
		KeyFrameQueue()
			: osg::Object(),
			m_currentKeyIndex(0)
		{
		}
		/** Copy constructor using CopyOp to manage deep vs shallow copy.*/
		KeyFrameQueue(const KeyFrameQueue& queue,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
			: osg::Object(queue, copyop),
			m_keyFrameQueue(queue.m_keyFrameQueue),
			m_currentKeyIndex(queue.m_currentKeyIndex)
		{
		}
		
		META_Box(hogbox,KeyFrameQueue);
				
		//adds a key to the queue, once the the key is reached
		//we smooth from current value to pos over duration seconds,
		//the template M is used to define the osgAnimation motion type
		//returns the new number of keys
		template <typename M>
		bool AddKey(const T& pos, const float& duration){
			KeyFrame frame;
			frame.end = pos;
			frame.duration = duration;
			frame.motion = new M(0.0f, duration, 1.0f, osgAnimation::Motion::CLAMP);
			m_keyFrameQueue.push_back(frame);
			
			//if this is the first key ensure m_start is set to current value
			return this->GetNumKeys();
		}
		
		//return a pointer to a specific key in the queue,
		//if the key does not exist then NULL is returned
		KeyFrame* GetKey(const unsigned int& index){
			if(index<0 || index >= m_keyFrameQueue.size()){return NULL;}
			typename std::deque<KeyFrame>::iterator itr = m_keyFrameQueue.begin();
			itr += index;
			return &(*itr);
		}
		//return the current front of the queue
		KeyFrame* GetCurrentKey(){
			return GetKey(0);
		}
		//return the current number of keys in the queue
		const unsigned int GetNumKeys(){ 
			return m_keyFrameQueue.size();
		}
		
		//remove a key from the queue
		bool RemoveKey(unsigned int& index){
			//check it's in range
			if(index < 0 || index >= m_keyFrameQueue.size()){return false;}
			m_keyFrameQueue.erase(m_keyFrameQueue.begin()+index);
			return true;
		}
		
		//pops the current key from the front, returns false
		//if there are no more keys in the queue 
		bool ChangeToNextKey(){
			m_keyFrameQueue.pop_front();
			//check the new size
			unsigned int size = m_keyFrameQueue.size();
			if(size > 0){return true;}
			return false;
		}
		
	protected:
		
		virtual ~KeyFrameQueue(){}
		
	protected:
		std::deque<KeyFrame> m_keyFrameQueue;
		unsigned int m_currentKeyIndex;
	};
	typedef osg::ref_ptr<KeyFrameQueue> KeyFrameQueuePtr;
	typedef std::map<std::string, KeyFrameQueuePtr> AnimationMap;
	
	
	//
	//adds a key to one of our animation queues (by animationName), once the the key is reached
	//we smooth from current m_value to pos over duration seconds,
	//the template M is used to define the osgAnimation motion type
	template <typename M>
	void AddKey(const T& pos, const float& duration, const std::string& animationName="DEFAULT"){
		KeyFrame frame;
		frame.end = pos;
		frame.duration = duration;
		frame.motion = new M(0.0f, duration, 1.0f, osgAnimation::Motion::CLAMP);
		m_keyFrameQueue.push_back(frame);

		//if this is the first key ensure m_start is set to current value
		if(this->GetNumKeys() == 1)
		{m_start = m_value;}
	}

	//return a pointer to a specific key in the queue,
	//if the key does not exist then NULL is returned
	KeyFrame* GetKey(const unsigned int& index, const std::string& animationName="DEFAULT"){
		return m_keyFrameQueue->GetKey(index);
	}
	//return the current front of the queue
	KeyFrame* GetCurrentKey(const std::string& animationName="DEFAULT"){
		return this->GetKey(0, animationName);
	}
	//return the current number of keys in the queue
	const unsigned int GetNumKeys(const std::string& animationName="DEFAULT"){ 
		return m_keyFrameQueue->GetNumKeys();
	}

	//remove a key from the queue
	bool RemoveKey(unsigned int& index, const std::string& animationName="DEFAULT"){
		return m_keyFrameQueue->RemoveKey(index);
	}

	//return the current actual value
	T GetValue(){return m_value;}
	//directly set the current value
	void SetValue(T value){m_value = value;m_start = value;}

	//
	//update toward the current keys end value by updating the osgAnimation
	//motion and using the value to interpolate between current start value 
	//and the keys end value. If we have reached the end of the keyframe queue
	//false is returned,
	bool Update(const float& timePassed){
		//get current key frame
		KeyFrame* key = this->GetCurrentKey();
		if(key)
		{	
			//update the motions time
			key->motion->update(timePassed);
			//get the interpolation value
			float t = key->motion->getValue();

			//interpolate from current start value to keys end value based on t
			T between = key->end - m_start;
			m_value = m_start + (between * t);

			//check if our motion has completed
			if(key->motion->getTime() >= key->motion->getDuration())
			{
				//move to next key if we have one
				this->ChangeToNextKey();
				return true;
			}
			return true;
		}else{
			return false;
		}
	}

protected:
	
	virtual ~AnimateValue(void){}

	//pops the current key from the front, returns false
	//if there are no more keys in the queue 
	bool ChangeToNextKey(){
		m_keyFrameQueue->ChangeToNextKey();//pop_front();
		//set the new start value to the current value
		m_start = m_value;
		//check the new size
		unsigned int size = m_keyFrameQueue->GetNumKeys();
		if(size > 0){return true;}
		return false;
	}

protected:

	//a map of pointers to the currently used animations/KeyFrameQueues
	//and their current weighting in te final Update value. 
	osg::ref_ptr<KeyFrameQueue> m_keyFrameQueue;
	
	//the lists of animation KeyFrameQueues index by a name string
	//the animation name "DEFAULT" is always added in the contructor to ensure
	//we have at least on animation to play on Update
	AnimationMap m_animations;

	//the value at the start of the key
	T m_start;

	//the current value
	T m_value;
};
	
//define all the common animate value types
typedef hogbox::AnimateValue<int> AnimateInt;
typedef osg::ref_ptr<AnimateInt> AnimateIntPtr;
typedef hogbox::AnimateValue<float> AnimateFloat;
typedef osg::ref_ptr<AnimateFloat> AnimateFloatPtr;
typedef hogbox::AnimateValue<osg::Vec2> AnimateVec2;
typedef osg::ref_ptr<AnimateVec2> AnimateVec2Ptr;
typedef hogbox::AnimateValue<osg::Vec3> AnimateVec3;
typedef osg::ref_ptr<AnimateVec3> AnimateVec3Ptr;
typedef hogbox::AnimateValue<osg::Vec4> AnimateVec4;
typedef osg::ref_ptr<AnimateVec4> AnimateVec4Ptr;
	
};
