#ifndef ORBITCAMERACOMPONENT_H_
#define ORBITCAMERACOMPONENT_H_

#include "Component/CameraComponent.h"
#include <gccore.h>
#include <wiiuse/wpad.h>

//@Makes camera orbit lookPoint, depending on input
class OrbitCameraComponent : public CameraComponent{
public:
	//Constructor
	//@Point to instances in PadSystem
	OrbitCameraComponent( guVector orbitOrigin, guVector * cam, guVector* look, float * pitch, float * yaw,
	 u16 * buttonsHeld, u16 * buttonsDown, u16 * buttonsUp, u16 * wButtonsHeld, u16 * wButtonsDown, u16 * wButtonsUp,
	 float * swing, float * wPitch, float * deviation, float * wRoll);
	virtual ~OrbitCameraComponent();
	virtual void OnStart();
	virtual void ComputeLogic (float dt);
	//Messaging
	virtual bool Receive( ComponentMessage msg );

	//Variables
	guVector m_orbitOrigin;
	float m_zoom;
};
#endif /*ORBITCAMERACOMPONENT_H_*/