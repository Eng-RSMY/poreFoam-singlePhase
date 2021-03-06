/*-------------------------------------------------------------------------*\
This code is part of poreFOAM, a suite of codes written using OpenFOAM
for direct simulation of flow at the pore scale. 	
You can redistribute this code and/or modify this code under the 
terms of the GNU General Public License (GPL) as published by the  
Free Software Foundation, either version 3 of the License, or (at 
your option) any later version. see <http://www.gnu.org/licenses/>.



The code has been developed by Ali Qaseminejad Raeini as a part his PhD 
at Imperial College London, under the supervision of Branko Bijeljic 
and Martin Blunt. 
* 
Please see our website for relavant literature:
http://www3.imperial.ac.uk/earthscienceandengineering/research/perm/porescalemodelling

For further information please contact us by email:
Ali Q Raeini:    a.qaseminejad-raeini09@imperial.ac.uk
Branko Bijeljic: b.bijeljic@imperial.ac.uk
Martin J Blunt:  m.blunt@imperial.ac.uk
\*-------------------------------------------------------------------------*/

#define SINGLE_PHASE
#define ifMonitor  if( runTime.timeIndex()%10== 0 ) 

#include "fvCFD.H"


//#include "singlePhaseTransportModel.H"
//#include "turbulentTransportModel.H"


#include "fixedFluxPressureFvPatchScalarField.H"

#include "pimpleControl.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //



int main(int argc, char *argv[])
{ 
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    if (!mesh.cells().size()) {Info<<"Error: no cells in (processor) mesh"<<endl; exit(-1);}
    pimpleControl pimple(mesh);
    #include "initContinuityErrs.H"
    #include "createFields.H"
    #include "createTimeControls.H"
    //#include "correctPhi.H"
    #include "CourantNo.H"

    //~ #include "setInitialDeltaT.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

	runTime.run();


    Info<< "\nStarting time loop\n" << endl;
#define 	curtail(a,b) (min (max((a),(b)),(1.-(b))))


	Info <<"min(p): "<<min(p)<<"max(p): "<<max(p)<<endl;
	scalar pRelaxF=0.1;




	Info << cBC<<endl<<endl<<endl;

	scalar tOld= runTime.elapsedCpuTime() ;

        //~ Info<<max(fvc::laplacian(p))<<min(fvc::laplacian(p))<<endl;
{
    surfaceScalarField muEff
    (
        IOobject
        (
            "muEff",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        rho*nu
        //+ fvc::interpolate(rho*turbulence->nut()) //caution turbulance is disabled
    );
	tmp<fvVectorMatrix> UEqn 
	(
		fvm::div(rho*phi, U)
	  - fvm::laplacian(muEff, U)
	);

	volScalarField rAU = 1.0/UEqn().A();
	surfaceScalarField rAUf = fvc::interpolate(rAU);
	setSnGrad<fixedFluxPressureFvPatchScalarField> ( p.boundaryFieldRef(),
		( phi.boundaryField()  // - fvOptions.relative(mesh.Sf().boundaryField() & U.boundaryField())
		) / (mesh.magSf().boundaryField()*rAUf.boundaryField())	);
}

{
	fvScalarMatrix pEqn
	(
		fvm::laplacian(p) 
	);
	pEqn.setReference(pRefCell, pRefValue);
	pEqn.solve(mesh.solver(p.name() + "Final"));
}




{
	fvScalarMatrix pEqn
	(
		fvm::laplacian(1.0e06/((fvc::grad(p)&fvc::grad(p))+0.001*average((fvc::grad(p)&fvc::grad(p)))),p)
	);
	pEqn.setReference(pRefCell, pRefValue);
	pEqn.solve(mesh.solver(p.name() + "Final"));
}

	
	Info<< "\n         Umax = " << max(mag(U)).value() << " m/s  "
	<< "Uavg = " << mag(average(U)).value() << " m/s"
	<< "   DP = " << (max(p)-min(p)).value() << " Pa"
	<< nl<< nl << endl;



{
	
	surfaceScalarField muEff
	(	IOobject ( "muEff", runTime.timeName(), mesh ),
		mesh, rho*nu
		//+ fvc::interpolate(rho*turbulence->nut()) //caution turbulance is disabled
	);



	//~ #include  "correctmuEff.H"
	solve
	(
		fvm::laplacian(muEff, U) - fvm::div(rho*phi, U) /*  - fvm::div(rho*phi, U) is just to avoid OpenFOAM bug with solver selection*/
		==
		fvc::grad(p)
	);	
	phi = (fvc::interpolate(U) & mesh.Sf());

	solve
	(
		fvm::laplacian(muEff, U) - fvm::div(rho*phi, U)
		==
		fvc::grad(p)
	);	
	phi = (fvc::interpolate(U) & mesh.Sf());
	
	Info<< "\n         Umax = " << max(mag(U)).value() << " m/s  "
	<< "Uavg = " << mag(average(U)).value() << " m/s"
	<< "   DP = " << (max(p)-min(p)).value() << " Pa"
	<< nl<< nl << endl;

	
	#include  "correctmuEff.H"
	solve
	(
		fvm::laplacian(muEff, U) - fvm::div(rho*phi, U)
		==
		fvc::grad(p)
	);	
	phi = (fvc::interpolate(U) & mesh.Sf());
}	


	Info<< "\n         Umax = " << max(mag(U)).value() << " m/s  "
	<< "Uavg = " << mag(average(U)).value() << " m/s"
	<< "   DP = " << (max(p)-min(p)).value() << " Pa"
	<< nl<< nl << endl;

	Info<<endl<<endl<<endl<<max(fvc::div(phi))<<endl<<endl;

for (int ii=1;ii<10;++ii)
{
	surfaceScalarField muEff
	(	IOobject ( "muEff", runTime.timeName(), mesh ),
		mesh, rho*nu
		//+ fvc::interpolate(rho*turbulence->nut()) //caution turbulance is disabled
	);
	#include  "correctmuEff.H"
	
	tmp<fvVectorMatrix> UEqn 
    (
		fvm::div(rho*phi, U) - fvm::laplacian(muEff, U)
    );
	scalar rlx=min((ii)/8.0,1.0);
    volScalarField rAU = 1.0/UEqn().A();
    surfaceScalarField rAUf = fvc::interpolate(rAU);
    //~ rAUf = rlx*rAUf+(1.0-rlx)*fvc::interpolate(fvc::average(rAUf));
    U = rAU*(UEqn().H());
    phi = (fvc::interpolate(U) & mesh.Sf());
	UEqn.clear();
	setSnGrad<fixedFluxPressureFvPatchScalarField> ( p.boundaryFieldRef(),
		( phi.boundaryField()  // - fvOptions.relative(mesh.Sf().boundaryField() & U.boundaryField())
		) / (mesh.magSf().boundaryField()*rAUf.boundaryField())	);

	for(int nonOrth=0; nonOrth<=pimple.nNonOrthCorr(); nonOrth++)
    { 
        fvScalarMatrix pEqn( fvm::laplacian(rAUf, p) 
              == rlx*fvc::div(phi) + (1.0-rlx)*fvc::laplacian(rAUf,p) );
        pEqn.setReference(pRefCell, pRefValue);
		pEqn.solve(mesh.solver(p.select(pimple.finalInnerIter())) );

		if (nonOrth == pimple.nNonOrthCorr())
            phi -= pEqn.flux();
    }
    U -= rAU*(fvc::grad(p));    
    U.correctBoundaryConditions();




	Info<< "\n         Umax = " << max(mag(U)).value() << " m/s  "
	<< "Uavg = " << mag(average(U)).value() << " m/s"
	<< "   DP = " << (max(p)-min(p)).value() << " Pa"
	<< nl<< nl << endl;
}


	Info<<endl<<max(fvc::div(phi))<<endl;

	for (int ii=1;ii<=2;++ii)
	{
		surfaceScalarField muEff
		(	IOobject ( "muEff", runTime.timeName(), mesh ),
			mesh, rho*nu	//+ fvc::interpolate(rho*turbulence->nut()) //caution turbulance is disabled
		);
		#include  "correctmuEff.H"

		tmp<fvVectorMatrix> UEqn 
		(
			fvm::div(rho*phi, U) - fvm::laplacian(muEff, U)
		);

		volScalarField rAU = 1.0/UEqn().A();
		surfaceScalarField rAUf = fvc::interpolate(rAU);
		U = rAU*(UEqn().H());
		phi = (fvc::interpolate(U) & mesh.Sf());
		UEqn.clear();

		setSnGrad<fixedFluxPressureFvPatchScalarField> ( p.boundaryFieldRef(),
			( phi.boundaryField()  // - fvOptions.relative(mesh.Sf().boundaryField() & U.boundaryField())
			) / (mesh.magSf().boundaryField()*rAUf.boundaryField())	);

		for(int nonOrth=0; nonOrth<=pimple.nNonOrthCorr(); nonOrth++)
		{
			fvScalarMatrix pEqn( fvm::laplacian(rAUf, p) == fvc::div(phi) );
			pEqn.setReference(pRefCell, pRefValue);
			pEqn.solve( mesh.solver(p.select(pimple.finalInnerIter())) );

			if (nonOrth == pimple.nNonOrthCorr())
				phi -= pEqn.flux();
		}
		U -= rAU*(fvc::grad(p));    
		U.correctBoundaryConditions();

	
		Info<< "\n         Umax = " << max(mag(U)).value() << " m/s  "
			<< "Uavg = " << mag(average(U)).value() << " m/s"
			<< "   DP = " << (max(p)-min(p)).value() << " Pa"
			<< nl<< nl << endl;
	}

	Info<<endl<<max(fvc::div(phi))<<endl;
	






U.write();
phi.write();

p.write();







Info<< "End\n" << endl;

return 0;
}


// ************************************************************************* //
