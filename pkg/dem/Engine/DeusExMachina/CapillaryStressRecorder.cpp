/*************************************************************************
*  Copyright (C) 2006 by luc scholt�                                    *
*  luc.scholtes@hmg.inpg.fr                                              *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "CapillaryStressRecorder.hpp"
#include <yade/pkg-common/RigidBodyParameters.hpp>
#include <yade/pkg-common/ParticleParameters.hpp>
#include <yade/pkg-dem/BodyMacroParameters.hpp>
#include <yade/pkg-dem/CapillaryParameters.hpp>
#include <yade/pkg-dem/CapillaryCohesiveLaw.hpp>
#include <yade/core/Omega.hpp>
#include <yade/core/MetaBody.hpp>
#include <yade/pkg-common/Sphere.hpp>
#include <boost/lexical_cast.hpp>


CapillaryStressRecorder::CapillaryStressRecorder () : DataRecorder()

{
	outputFile = "";
	interval = 1;
	sphere_ptr = shared_ptr<GeometricalModel> (new Sphere);
	SpheresClassIndex = sphere_ptr->getClassIndex();
	height = 0;
	width = 0;
	depth = 0;
	thickness = 0;
	
// 	upperCorner = Vector3r(0,0,0);
// 	lowerCorner = Vector3r(0,0,0);
	
}


void CapillaryStressRecorder::postProcessAttributes(bool deserializing)
{
	if(deserializing)
	{
		ofile.open(outputFile.c_str());
	}
}


void CapillaryStressRecorder::registerAttributes()
{
	DataRecorder::registerAttributes();
	REGISTER_ATTRIBUTE(outputFile);
	REGISTER_ATTRIBUTE(interval);
	
	REGISTER_ATTRIBUTE(wall_bottom_id);
 	REGISTER_ATTRIBUTE(wall_top_id);
 	REGISTER_ATTRIBUTE(wall_left_id);
 	REGISTER_ATTRIBUTE(wall_right_id);
 	REGISTER_ATTRIBUTE(wall_front_id);
 	REGISTER_ATTRIBUTE(wall_back_id);
 	
 	REGISTER_ATTRIBUTE(height);
	REGISTER_ATTRIBUTE(width);
	REGISTER_ATTRIBUTE(depth);
	REGISTER_ATTRIBUTE(thickness);
	
// 	REGISTER_ATTRIBUTE(upperCorner);
// 	REGISTER_ATTRIBUTE(lowerCorner);
	

}


bool CapillaryStressRecorder::isActivated()
{
	return ((Omega::instance().getCurrentIteration() % interval == 0) && (ofile)); // active le truc tout les "interval" !??
}


void CapillaryStressRecorder::action(Body * body)
{
	MetaBody * ncb = static_cast<MetaBody*>(body);
	shared_ptr<BodyContainer>& bodies = ncb->bodies;
		
	Real f1_cap_x=0, f1_cap_y=0, f1_cap_z=0, x1=0, y1=0, z1=0, x2=0, y2=0, z2=0;
	
	Real sig11_cap=0, sig22_cap=0, sig33_cap=0, sig12_cap=0, sig13_cap=0,
	sig23_cap=0, Vwater = 0, CapillaryPressure = 0;
	
	InteractionContainer::iterator ii    = ncb->transientInteractions->begin();
        InteractionContainer::iterator iiEnd = ncb->transientInteractions->end();
        
        int j = 0;
        
        for(  ; ii!=iiEnd ; ++ii ) 
        {
                if ((*ii)->isReal)
                {	
                	const shared_ptr<Interaction>& interaction = *ii;
                
                	CapillaryParameters* meniscusParameters		= static_cast<CapillaryParameters*>(interaction->interactionPhysics.get());
                        
                        if (meniscusParameters->meniscus)
                        {
                	
                	j=j+1;
                	
                        unsigned int id1 = interaction -> getId1();
			unsigned int id2 = interaction -> getId2();
			
			Vector3r fcap = meniscusParameters->Fcap;
			
			f1_cap_x=fcap[0];
			f1_cap_y=fcap[1];
			f1_cap_z=fcap[2];
			
			BodyMacroParameters* de1 		= static_cast<BodyMacroParameters*>((*bodies)[id1]->physicalParameters.get());
			x1 = de1->se3.position[0];
			y1 = de1->se3.position[1];
			z1 = de1->se3.position[2];


			BodyMacroParameters* de2 		= static_cast<BodyMacroParameters*>((*bodies)[id2]->physicalParameters.get());
			x2 = de2->se3.position[0];
			y2 = de2->se3.position[1];
			z2 = de2->se3.position[2];

			///Calcul des contraintes capillaires "locales"
			
			sig11_cap = sig11_cap + f1_cap_x*(x1 - x2);
			sig22_cap = sig22_cap + f1_cap_y*(y1 - y2);
			sig33_cap = sig33_cap + f1_cap_z*(z1 - z2);
			sig12_cap = sig12_cap + f1_cap_x*(y1 - y2);
			sig13_cap = sig13_cap + f1_cap_x*(z1 - z2);
			sig23_cap = sig23_cap + f1_cap_y*(z1 - z2);
			
			/// Calcul du volume d'eau
			
 			Vwater += meniscusParameters->Vmeniscus;
 			CapillaryPressure=meniscusParameters->CapillaryPressure;
			
			}
			
                }
        }	

// 	if (Omega::instance().getCurrentIteration() % 10 == 0) 
// 	{cerr << "interactions capillaires = " << j << endl;}

	/// volume de l'�hantillon
	
	PhysicalParameters* p_bottom =
static_cast<PhysicalParameters*>((*bodies)[wall_bottom_id]->physicalParameters.
get());
	PhysicalParameters* p_top   =	
static_cast<PhysicalParameters*>((*bodies)[wall_top_id]->physicalParameters.get(
));
	PhysicalParameters* p_left =
static_cast<PhysicalParameters*>((*bodies)[wall_left_id]->physicalParameters.get
());
	PhysicalParameters* p_right =
static_cast<PhysicalParameters*>((*bodies)[wall_right_id]->physicalParameters.
get());
	PhysicalParameters* p_front =
static_cast<PhysicalParameters*>((*bodies)[wall_front_id]->physicalParameters.
get());
	PhysicalParameters* p_back =
static_cast<PhysicalParameters*>((*bodies)[wall_back_id]->physicalParameters.get
());
	
	
	height = p_top->se3.position.Y() - p_bottom->se3.position.Y() -
thickness;
	width = p_right->se3.position.X() - p_left->se3.position.X() -
thickness;
	depth = p_front->se3.position.Z() - p_back->se3.position.Z() -
thickness;

	Real Vech = (height) * (width) * (depth);

	// degr�de saturation/porosit�	
	BodyContainer::iterator bi = bodies->begin();
	BodyContainer::iterator biEnd = bodies->end();
	
	Real Vs = 0, Rbody = 0, SR = 0;
// 	int n = 0;
	
	for ( ; bi!=biEnd; ++bi) 
	
	{	
		shared_ptr<Body> b = *bi;
		
		int geometryIndex = b->geometricalModel->getClassIndex();
		//cerr << "model = " << geometryIndex << endl;
		
		if (geometryIndex == SpheresClassIndex)
		{
			Sphere* sphere =
		static_cast<Sphere*>(b->geometricalModel.get());
			Rbody = sphere->radius;
			SR+=Rbody;
			
			Vs += 1.333*Mathr::PI*(Rbody*Rbody*Rbody);
		}
	}
	
// 	Real Rmoy = SR/N;
// 	Real V = (height-2*Rmoy) * (width-2*Rmoy) * (depth-2*Rmoy);
	
	Real Vv = Vech - Vs;
	
// 	cerr << "Vw = " << Vwater << "Vv = " << Vv << endl;
// 	cerr << "V = " << V << "Vs = " << Vs << endl;
	
//	Real n = Vv/Vech;
	Real Sr = 100*Vwater/Vv;
	if (Sr>100) Sr=100;
	Real w = 100*Vwater/Vech;
	if (w>(100*Vv/Vech)) w=100*(Vv/Vech);
	
	/// Calcul des contraintes "globales"
	
	Real SIG_11_cap=0, SIG_22_cap=0, SIG_33_cap=0, SIG_12_cap=0,
	SIG_13_cap=0, SIG_23_cap=0;
	
	SIG_11_cap = sig11_cap/Vech;
	SIG_22_cap = sig22_cap/Vech;
	SIG_33_cap = sig33_cap/Vech;
	SIG_12_cap = sig12_cap/Vech;
	SIG_13_cap = sig13_cap/Vech;
	SIG_23_cap = sig23_cap/Vech;
	
// 	// calcul des d�ormations
// 	
// 	Real EPS_11=0, EPS_22=0, EPS_33=0;
// 	
// 	Real width_0 = upperCorner[0]-lowerCorner[0], height_0 =
// 	upperCorner[1]-lowerCorner[1],
// 	depth_0 = upperCorner[2]-lowerCorner[2];
// 	
// 	//cerr << "width_0 = " << width_0 << " width = " << width << endl;
// 	
// 	EPS_11 = (width_0 - width)/width_0;
// 	EPS_22 = (height_0 - height)/height_0;
// 	EPS_33 = (depth_0 - depth)/depth_0;
	
// 	if (Omega::instance().getCurrentIteration() % 100 == 0) 
// 	{cerr << "Vwater = " << Vwater;
// 	cerr << " | CapillaryPressure= " << CapillaryPressure << " | Sr= " << Sr
// 	<<endl;}

	ofile << lexical_cast<string>(Omega::instance().getSimulationTime()) << " " 
		<< lexical_cast<string>(SIG_11_cap) << " " 
		<< lexical_cast<string>(SIG_22_cap) << " " 
		<< lexical_cast<string>(SIG_33_cap) << " " 
		<< lexical_cast<string>(SIG_12_cap) << " "
		<< lexical_cast<string>(SIG_13_cap)<< " "
		<< lexical_cast<string>(SIG_23_cap)<< "   "
// 		<< lexical_cast<string>(EPS_11) << " "
// 		<< lexical_cast<string>(EPS_22) << " "
// 		<< lexical_cast<string>(EPS_33) << "   "
		<< lexical_cast<string>(CapillaryPressure) << " "
		<< lexical_cast<string>(Sr)<< " " 
		<< lexical_cast<string>(w)<< " "
		<< endl;
	
}

