#include "System\PhysicSystem.h"
#include "System\ObjectSystem.h"
#include "System\GraphicSystem.h"
#include "System\PadSystem.h"
#include "Sphere.h"
#include "BoundingBox.h"

#include "Extra/Math.h"

#include <iomanip> // setprecision
#include <sstream> // stringstream

using namespace std;

//Instance
PhysicSystem * PhysicSystem::m_instance = NULL;
PhysicSystem * PhysicSystem::GetInstance()
{
	//Singleton
	if (m_instance == NULL)
	{
		m_instance = new PhysicSystem();
	}
	return m_instance;
}
//Constructor
PhysicSystem::PhysicSystem()
{
	m_gravity = 9.8f;
	//Viscosity for earth's air @  0'Celsius = 1.33*10^-5 kg/ms^2
	m_airViscosity = 0.133f;
	m_minDt = 1.0f / 60.0f;
	m_accumulator = 0;
	m_frictionCoefficient = 0.03f;
	m_stepMode = false;
	m_stepOnce = false;
}
//Destructor
PhysicSystem::~PhysicSystem()
{

}
///Init
void PhysicSystem::Initialize()
{
	//@What do here?
}
///Timestep and run physics

void PhysicSystem::Update(float dt)
{	

	if (m_stepMode) {
		if (m_stepOnce) {
			UpdatePhysics(m_minDt);
			m_stepOnce = false;
		}
	}
	else
	{
		//@Timestep
		//https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-core-engine--gamedev-7493
		m_accumulator += dt;

		// @Avoid spiral of death and clamp dt, thus clamping
		// how many times the UpdatePhysics can be called in
		// a single game loop.
		if (m_accumulator > 0.2f) m_accumulator = 0.2f;

		while (m_accumulator > m_minDt) {
			UpdatePhysics(m_minDt);
			m_accumulator -= m_minDt;
		}

		//@To create a lerp between this frame and the next, interact with the graphic system.
		//ApproxTransform.position = transform.position + m_velocity*m_accumulator ?
		//float alpha = m_accumulator / m_minDt;
	}
}
//@Messaging
void PhysicSystem::SendMessage(ComponentMessage msg)
{

}
///Utility
///Physics loop
void PhysicSystem::UpdatePhysics(float dt) {

	//GraphicSystem * gs = GraphicSystem::GetInstance();
	vector<RigidbodyComponent*> m_rigidbodies = ObjectSystem::GetInstance()->GetRigidbodyComponentList();
	vector<pair<RigidbodyComponent*, RigidbodyComponent*>> m_pairs;
	
	/*@@LIST OF VECTOR GU COMMANDS
	void c_guVecAdd(guVector *a,guVector *b,guVector *ab);
	void c_guVecSub(guVector *a,guVector *b,guVector *ab);
	void c_guVecScale(guVector *src,guVector *dst,f32 scale);
	void c_guVecNormalize(guVector *v);
	void c_guVecMultiply(Mtx mt,guVector *src,guVector *dst);
	void c_guVecCross(guVector *a,guVector *b,guVector *axb);
	void c_guVecMultiplySR(Mtx mt,guVector *src,guVector *dst);
	f32 c_guVecDotProduct(guVector *a,guVector *b);
	*/

	//@First loop: Integration + First culling algorithm
	for (unsigned int i = 0; i < m_rigidbodies.size(); i++) {
		RigidbodyComponent* currentRb = m_rigidbodies[i];
		//@Integration
		if (currentRb->m_isKinematic) {
			currentRb->m_acceleration = Math::VecZero;
			currentRb->m_velocity = Math::VecZero;
		}
		else
		{
			TransformComponent * t = &currentRb->m_owner->m_transform;
			if (Math::LengthSq(currentRb->m_velocity) > m_frictionCoefficient*m_frictionCoefficient){
				currentRb->m_isSleeping = false;
			}
			else {
				currentRb->m_isSleeping = true;
				currentRb->m_velocity = Math::VecZero;
			}
			if (!currentRb->m_isSleeping){

				guVector airViscosityForce;
				guVecScale( &currentRb->m_velocity, &airViscosityForce, m_airViscosity );
				guVecSub( &currentRb->m_force, &airViscosityForce, &currentRb->m_force );
				//currentRb->m_force -= m_airViscosity * currentRb->m_velocity;
				//@BAKED KINETIC FRICTION SINCE WE ASSUMED WE'RE COLLIDING WITH THE TABLE'S PLANE DUE TO GRAVITY
				//@Friction:
				//Calculate normal force from impulse
				//f = m*a
				//f = mdv/dt
				float forceAgainstTable = m_gravity*currentRb->m_mass;
				guVector velNorm = currentRb->m_velocity;

				guVecNormalize(&velNorm);

				guVector friction;
				guVecScale( &velNorm, &friction, -abs(forceAgainstTable)*m_frictionCoefficient );
				guVecAdd( &currentRb->m_force, &friction, &currentRb->m_force);

				//@Derived rotation from movement
				guVector rotVector;
				guVector up = guVector { 0, 1, 0 };	
				guVecCross(&velNorm, &up, &rotVector);
				guQuaternion rotation = Math::QuatFromAxisAngle( rotVector, Math::Length(currentRb->m_velocity)/20);
				guQuatMultiply( &t->m_rotation, &rotation, &t->m_rotation);
			}
			//@INTEGRATE
			guVecScale( &currentRb->m_force, &currentRb->m_acceleration, 1 / currentRb->m_mass );
			//currentRb->m_acceleration = currentRb->m_force / currentRb->m_mass;
			guVector accTimesDt;
			guVecScale( &currentRb->m_acceleration, &accTimesDt, dt );
			guVecAdd( &currentRb->m_velocity, &accTimesDt, &currentRb->m_velocity);
			//currentRb->m_velocity += currentRb->m_acceleration*dt;
			guVector velTimesDt;
			guVecScale( &currentRb->m_velocity, &velTimesDt, dt);
			guVecAdd( &currentRb->m_owner->m_transform.m_position, &velTimesDt, &currentRb->m_owner->m_transform.m_position);
			//currentRb->m_owner->m_transform.m_position += currentRb->m_velocity*dt;
			currentRb->m_force = Math::VecZero;

			/*PRINTPOS*/
			//gs->AddLog(to_string(currentRb->m_velocity.z));
			//gs->AddLog(to_string(currentRb->m_velocity.y));
			//gs->AddLog(to_string(currentRb->m_velocity.x));
			//gs->AddLog( currentRb->m_owner->m_name + " velocity:");
			//if (currentRb->m_isSleeping) gs->AddLog(currentRb->m_owner->m_name + " is sleeping");
		}

		//@SSScheme
		for (unsigned int j = i + 1; j < m_rigidbodies.size(); j++) {
			//@To avoid double checks, we only check upwards
			//If (SpaceSubdivisionBinIsEqual)
			m_pairs.push_back(make_pair(currentRb, m_rigidbodies[j]));
		}
	}

	//@Start nulling out collider pairs
	
	//@Medium Phase

	///@Surviving pairs MUST be colliding.
	
	//@Narrow Phase
	for (u16 i = 0; i < m_pairs.size(); i++) {
		NarrowPhase(m_pairs[i].first, m_pairs[i].second, dt);
	}
}
///Narrow phase component
bool PhysicSystem::NarrowPhase(RigidbodyComponent * rb1, RigidbodyComponent * rb2, float dt) {

	//@Cases?
	//Resting contact, moving contact, contact vs kinematic
	//@Impulse based collision response
	if (rb1->m_isKinematic && rb2->m_isKinematic) return false; //No collisions between AABBs
	switch (rb1->m_shape->m_type) {
		case ShapeType::SPHERE:
		{
			switch (rb2->m_shape->m_type) {
				case ShapeType::SPHERE:
				{
					 return SphereToSphere(rb1, rb2, dt);
				}
				break;
				case ShapeType::AABB:
				{
					return SphereToAABB(rb1, rb2, dt);
				}
				break;
			}
		}
		break;
		case ShapeType::AABB:
		{
			switch (rb2->m_shape->m_type) {
				case ShapeType::SPHERE:
				{
					return SphereToAABB(rb2, rb1, dt);
				}
				break;
				case ShapeType::AABB:
				{
					return 0;//@AABB Collisions nonhandled since they're either kinematic or triggers, and we dont need to trigger anything
				}
				break;
			}
		}
		break;
	}
	return false;
}
//Collision queries
bool PhysicSystem::SphereToSphere(RigidbodyComponent * rb1, RigidbodyComponent * rb2, float dt){

	//@At this point it can be a static_cast
	Sphere * sphere1 = dynamic_cast<Sphere*>(rb1->m_shape);
	Sphere * sphere2 = dynamic_cast<Sphere*>(rb2->m_shape);
	TransformComponent * t1 = &rb1->m_owner->m_transform;
	TransformComponent * t2 = &rb2->m_owner->m_transform;
	//@Cases?
	//Resting contact, moving contact, contact vs kinematic
	//@Impulse based collision response
	//@1:Are they colliding?
	float distSq = Math::DistSq(t1->m_position, t2->m_position);
	// Calculate the sum of the radii, then square it
	float sumRadiiSq = sphere1->m_radius + sphere2->m_radius;
	sumRadiiSq *= sumRadiiSq;
	if (distSq <= sumRadiiSq) {
		// A and B are touching
		//We provide generic info such as: Necessary contact points, each with penetration depth scalar, a common normal vector, and rigidbodies involved.
		//@CONVENTION: Normal always points to first rigidbody pair.
		//@Overlap is always positive
		float dist = sqrtf(distSq);
		float overlap = (sphere1->m_radius + sphere2->m_radius - dist);
		guVector normal;
		guVecSub( &t1->m_position, &t2->m_position, &normal);
		guVecScale( &normal, &normal, 1/dist );

		//@Dirk Gregorious: Contact point is middle point of two surfaces
		//@We only care about storing contactPoints for angular impulse.
		/*guVector surfacePoint1;
		guVector normalTimesRadius1;
		guVecScale( &normal, &normalTimesRadius1, sphere1->m_radius);
		guVecSub( &t1->m_position, &normalTimesRadius1, &surfacePoint1 );

		guVector surfacePoint2;
		guVector normalTimesRadius2;
		guVecScale( &normal, &normalTimesRadius2, sphere2->m_radius);
		guVecSub( &t2->m_position, &normalTimesRadius2, &surfacePoint2 );

		guVector contactPoint;
		guVecSub( &surfacePoint2, &surfacePoint1, &contactPoint);
		guVecScale( &contactPoint, &contactPoint, 0.5f);
		guVecAdd( &contactPoint, &surfacePoint1, &contactPoint);*/

		///1:Displacement
		//@Static collision resolution based on speed
		float v1Length = Math::Length(rb1->m_velocity);
		float v2Length = Math::Length(rb2->m_velocity);
		if ( v1Length + v2Length <= __FLT_EPSILON__){
			//@Displace 0.5 each
			guVector normalMultiplied;
			guVecScale(&normal, &normalMultiplied, overlap*0.5f);
			guVecAdd( &t1->m_position, &normalMultiplied, &t1->m_position);
			guVecSub( &t2->m_position, &normalMultiplied, &t2->m_position);
			return true;
		}
		else{
			//@What if two objects with no velocity just collided?
			float v1Ratio = v1Length / (v1Length + v2Length);
			float v2Ratio = v2Length / (v1Length + v2Length);
			
			if (!rb1->m_isKinematic){
				guVector normalMultiplied;
				guVecScale( &normal, &normalMultiplied, overlap*v1Ratio );
				guVecAdd( &t1->m_position, &normalMultiplied, &t1->m_position );
			}
			if (!rb2->m_isKinematic){
				guVector normalMultiplied;
				guVecScale( &normal, &normalMultiplied, overlap*v2Ratio );
				guVecSub( &t2->m_position, &normalMultiplied, &t2->m_position );
			}
		}

		///2:Dynamic resolution
		//http://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php?page=3
		// First, find the normalized vector n from the center of
		// circle1 to the center of circle2

		// Find the length of the component of each of the movement
		// vectors along n.
		// a1 = v1 . n
		// a2 = v2 . n

		float a1 = guVecDotProduct( &rb1->m_velocity, &normal );
		float a2 = guVecDotProduct( &rb2->m_velocity, &normal );

		// Using the optimized version,
		// optimizedP =  2(a1 - a2)
		//              -----------
		//                m1 + m2
		float optimizedP = (2.0f * (a1 - a2)) / (rb1->m_mass + rb2->m_mass);


		// Calculate v1', the new movement vector of circle1
		// v1' = v1 - optimizedP * m2 * n
		guVector velDelta1;
		guVecScale( &normal, &velDelta1, optimizedP*rb2->m_mass);
		guVecSub( &rb1->m_velocity, &velDelta1, &rb1->m_velocity);

		guVector velDelta2;
		guVecScale( &normal, &velDelta2, optimizedP*rb1->m_mass);
		guVecAdd( &rb2->m_velocity, &velDelta2, &rb2->m_velocity);

		//3: Friction::Because of our Impulse based resolution, we need to calculate the normal force 'After the fact'
		//Same goes for torque?
		//Flinear = F
		//Ftorque = F x(p - x)

		//@Friction:
		//Calculate normal force from impulse
		//f = m*a
		//f = mdv/dt
		guVector rb1Force;
		guVecScale( &velDelta1, &rb1Force, rb1->m_mass/dt);
		//Find component of force along normal
		float normalForce = guVecDotProduct( &rb1Force, &normal );//Dot normalized, or not?

		guVector rb2Force;
		guVecScale( &velDelta2, &rb2Force, rb2->m_mass/dt);
		float normal2Force = guVecDotProduct( &rb2Force, &normal );

		guVector vel1Norm = rb1->m_velocity;
		guVecNormalize(&vel1Norm);

		guVector vel2Norm = rb2->m_velocity;
		guVecNormalize( &vel2Norm );

		guVector friction1;
		if (rb1->m_isSleeping){
			//Static friction model:
			//Friction is twice the m_frictionCoefficient
			guVecScale(&vel1Norm, &friction1, -abs(normalForce)*m_frictionCoefficient*2);
			if (Math::LengthSq(friction1) > Math::LengthSq(rb1Force)){
				//Scale friction to be equal to rb1Force, on the opposite direction
				guVecScale(&rb1Force, &friction1, -1);
			}
		}
		else{
			//Kinetic friction
			guVecScale( &vel1Norm, &friction1, -abs(normalForce)*m_frictionCoefficient );
		}
		guVecAdd( &rb1->m_force, &friction1, &rb1->m_force);


		guVector friction2;
		if (rb2->m_isSleeping){
			//Static friction model:
			//Friction is twice the m_frictionCoefficient
			guVecScale(&vel2Norm, &friction2, -abs(normalForce)*m_frictionCoefficient*2);
			if (Math::LengthSq(friction2) > Math::LengthSq(rb2Force)){
				guVecScale(&rb2Force, &friction2, -1);
			}
		}
		else{
			guVecScale( &vel2Norm, &friction2, -abs(normal2Force)*m_frictionCoefficient );
		}
		guVecAdd( &rb2->m_force, &friction2, &rb2->m_force);
		
		//@Put to rest?
		return true;
	}
	return false;
}
//Sphere-AABB
bool PhysicSystem::SphereToAABB(RigidbodyComponent * rb1, RigidbodyComponent * rb2, float dt ){
	//@At this point it can be a static_cast
	Sphere * sphere1 = dynamic_cast<Sphere*>(rb1->m_shape);

	TransformComponent * t1 = &rb1->m_owner->m_transform;
	TransformComponent * t2 = &rb2->m_owner->m_transform;
	guVector closestPoint = ClosestPtPointAABB(t1->m_position, rb2);

	float distSq = Math::DistSq(t1->m_position, closestPoint);
	if (distSq <= sphere1->m_radius*sphere1->m_radius){
		GraphicSystem * gs = GraphicSystem::GetInstance();
		//gs->AddLog(to_string(closestPoint.z));
		//gs->AddLog(to_string(closestPoint.y));
		//gs->AddLog(to_string(closestPoint.x));
		//gs->AddLog("Sphere-AABB collision at:");
		if (rb2->m_isTrigger){
			//@We just send necessary messages
			rb1->m_owner->m_isDeleted = true;
			gs->AddLog("Ball hit trigger");
			if (rb1->m_ballType == BallType::BALL_NONE){
				PadSystem::GetInstance()->SendMessage(ComponentMessage::WHITE_IN_POT);
			}
			//Message to pad system, for ball scored
			if (rb1->m_ballType == BallType::BALL_RED) {
				PadSystem::GetInstance()->SendMessage(ComponentMessage::PLAYER_RED_SCORED);
				GraphicSystem::GetInstance()->SendMessage(ComponentMessage::PLAYER_RED_SCORED);
			}
			if (rb1->m_ballType == BallType::BALL_BLUE) {
				PadSystem::GetInstance()->SendMessage(ComponentMessage::PLAYER_BLUE_SCORED);
				GraphicSystem::GetInstance()->SendMessage(ComponentMessage::PLAYER_BLUE_SCORED);//Adds point to right player
			}
			return true;
		}
		else{
			//They collide
			//@We assume and only solve for SHALLOW penetration
			//Collision normal is literally closestPoint to sphereCenter
			float dist = sqrtf(distSq);
			float overlap = sphere1->m_radius - dist;

			guVector normal;
			guVecSub( &t1->m_position, &closestPoint, &normal);
			guVecScale( &normal, &normal, 1/dist );


			//@Dirk Gregorious: Contact point is middle point of two surfaces
			//@We only care about storing contactPoints for angular impulse.
			/*guVector contactPoint = closestPoint;*/
			///1:Displacement
			//@Static collision resolution based on speed
			float v1Length = Math::Length(rb1->m_velocity);
			float v2Length = Math::Length(rb2->m_velocity);
			//@What if two objects with no velocity just collided?
			float v1Ratio = v1Length / (v1Length + v2Length);
			float v2Ratio = v2Length / (v1Length + v2Length);
				
			if (!rb1->m_isKinematic){
				guVector normalMultiplied;
				guVecScale( &normal, &normalMultiplied, overlap*v1Ratio );
				guVecAdd( &t1->m_position, &normalMultiplied, &t1->m_position );
			}
			if (!rb2->m_isKinematic){
				guVector normalMultiplied;
				guVecScale( &normal, &normalMultiplied, overlap*v2Ratio );
				guVecSub( &t2->m_position, &normalMultiplied, &t2->m_position );
			}

		///2:Dynamic resolution
		//http://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php?page=3
		// First, find the normalized vector n from the center of
		// circle1 to the center of circle2

		// Find the length of the component of each of the movement
		// vectors along n.
		// a1 = v1 . n
		// a2 = v2 . n

		float a1 = guVecDotProduct( &rb1->m_velocity, &normal );
		float a2 = guVecDotProduct( &rb2->m_velocity, &normal );

		// Using the optimized version,
		// optimizedP =  2(a1 - a2)
		//              -----------
		//                m1 + m2
		float optimizedP = (2.0f * (a1 - a2)) / (rb1->m_mass + rb2->m_mass);


		// Calculate v1', the new movement vector of circle1
		// v1' = v1 - optimizedP * m2 * n
		guVector velDelta1;
		guVecScale( &normal, &velDelta1, optimizedP*rb2->m_mass);
		guVecSub( &rb1->m_velocity, &velDelta1, &rb1->m_velocity);

		guVector velDelta2;
		guVecScale( &normal, &velDelta2, optimizedP*rb1->m_mass);
		guVecAdd( &rb2->m_velocity, &velDelta2, &rb2->m_velocity);

		//3: Friction::Because of our Impulse based resolution, we need to calculate the normal force 'After the fact'
		//Same goes for torque?
		//Flinear = F
		//Ftorque = F x(p - x)

		//@Friction:
		//Calculate normal force from impulse
		//f = m*a
		//f = mdv/dt
		guVector rb1Force;
		guVecScale( &velDelta1, &rb1Force, rb1->m_mass/dt);
		//Find component of force along normal
		float normalForce = guVecDotProduct( &rb1Force, &normal );//Dot normalized, or not?

		guVector rb2Force;
		guVecScale( &velDelta2, &rb2Force, rb2->m_mass/dt);
		float normal2Force = guVecDotProduct( &rb2Force, &normal );

		guVector vel1Norm = rb1->m_velocity;
		guVecNormalize(&vel1Norm);

		guVector vel2Norm = rb2->m_velocity;
		guVecNormalize( &vel2Norm );

		guVector friction1;
		guVecScale( &vel1Norm, &friction1, -abs(normalForce)*m_frictionCoefficient );
		guVecAdd( &rb1->m_force, &friction1, &rb1->m_force);

		guVector friction2;
		guVecScale( &vel2Norm, &friction2, -abs(normal2Force)*m_frictionCoefficient );
		guVecAdd( &rb2->m_force, &friction2, &rb2->m_force);

		}
	}
	return false;
}
//Helpful queries
guVector PhysicSystem::ClosestPtPointAABB(guVector point, RigidbodyComponent * rb){
	// Given point p, return the point q on or in AABB b that is closest to p
	float p[3] = { point.x, point.y, point.z };//p
	guVector tpos = rb->m_owner->m_transform.m_position;
	BoundingBox * AABB = dynamic_cast<BoundingBox*>(rb->m_shape);

	float min[3] = { tpos.x - AABB->m_halfExtents.x, tpos.y - AABB->m_halfExtents.y, tpos.z - AABB->m_halfExtents.z };
	float max[3] = { tpos.x + AABB->m_halfExtents.x, tpos.y + AABB->m_halfExtents.y, tpos.z + AABB->m_halfExtents.z };
	// For each coordinate axis, if the point coordinate value is
	// outside box, clamp it to the box, else keep it as is
	for (int i = 0; i < 3;i++) { 
		float v = p[i];
		 if (v < min[i]) v = min[i]; // v = max(v, b.min[i])
		 if (v > max[i]) v = max[i]; // v = min(v, b.max[i])
		  p[i] = v;
	}
	return guVector{p[0], p[1], p[2]};
}