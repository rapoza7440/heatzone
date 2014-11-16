//---------------------------------------------------------------------------
//....................   FDSTAG VECTOR OUTPUT ROUTINES   ....................
//---------------------------------------------------------------------------
#include "LaMEM.h"
#include "fdstag.h"
#include "solVar.h"
#include "scaling.h"
#include "tssolve.h"
#include "bc.h"
#include "JacRes.h"
#include "paraViewOutBin.h"
#include "outFunct.h"
#include "interpolate.h"
//---------------------------------------------------------------------------
// WARNING!
//
// ParaView symmetric tensor components ordering is: xx, yy, zz, xy, yz, xz
//
// This is diagonal (rather than row-wise) storage format !!!
//
// As usual, this isn't documented anywhere !!! Take care of this in future versions.
//---------------------------------------------------------------------------
// interpolation function header
#define INTERPOLATION_FUNCTION_HEADER \
	FDSTAG      *fs; \
	Scaling     *scal; \
	PetscScalar ***buff, cf; \
	PetscInt     i, j, k, nx, ny, nz, sx, sy, sz, iter; \
	InterpFlags  iflag; \
	PetscErrorCode ierr; \
	PetscFunctionBegin; \
	fs   = outbuf->fs; \
	scal = &jr->scal; \
	iflag.update    = PETSC_FALSE; \
	iflag.use_bound = PETSC_FALSE;
//---------------------------------------------------------------------------
// access function header
#define ACCESS_FUNCTION_HEADER \
	PetscScalar cf; \
	Scaling     *scal; \
	InterpFlags  iflag; \
	PetscErrorCode ierr; \
	PetscFunctionBegin; \
	scal = &jr->scal; \
	iflag.update    = PETSC_FALSE; \
	iflag.use_bound = PETSC_FALSE;
//---------------------------------------------------------------------------
#define INTERPOLATE(da, vec, _IFUNCT_, FIELD) \
	ierr = DMDAGetCorners (da, &sx, &sy, &sz, &nx, &ny, &nz); CHKERRQ(ierr); \
	ierr = DMDAVecGetArray(da, vec, &buff); CHKERRQ(ierr); \
	START_STD_LOOP \
		FIELD \
	END_STD_LOOP \
	ierr = DMDAVecRestoreArray(da, vec, &buff); CHKERRQ(ierr); \
	LOCAL_TO_LOCAL(da, vec) \
	ierr = _IFUNCT_(fs, vec, outbuf->lbcor, iflag); CHKERRQ(ierr);
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWritePhase"
PetscErrorCode PVOutWritePhase(JacRes *jr, OutBuf *outbuf)
{
	Material_t  *phases;
	PetscScalar *phRat, mID;
	PetscInt     jj, numPhases;

	INTERPOLATION_FUNCTION_HEADER

	// macro to copy phase parameter to buffer
	#define GET_PHASE \
		phRat = jr->svCell[iter++].phRat; \
		mID = 0.0; \
		for(jj = 0; jj < numPhases; jj++) \
			mID += phRat[jj]*phases[jj].ID; \
		buff[k][j][i] = mID;

	// no scaling is necessary for the phase
	cf = scal->unit;

	// access material parameters
	phases    = jr->phases;
	numPhases = jr->numPhases;

	INTERPOLATE(fs->DA_CEN, outbuf->lbcen, InterpCenterCorner, GET_PHASE)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteDensity"
PetscErrorCode PVOutWriteDensity(JacRes *jr, OutBuf *outbuf)
{
	INTERPOLATION_FUNCTION_HEADER

	// macro to copy density to buffer
	#define GET_DENSITY buff[k][j][i] = jr->svCell[iter++].svBulk.rho;

	cf = scal->density;

	INTERPOLATE(fs->DA_CEN, outbuf->lbcen, InterpCenterCorner, GET_DENSITY)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteViscosity"
PetscErrorCode PVOutWriteViscosity(JacRes *jr, OutBuf *outbuf)
{
	INTERPOLATION_FUNCTION_HEADER

	// macro to copy viscosity to buffer
	#define GET_VISCOSITY buff[k][j][i] = jr->svCell[iter++].svDev.eta;

	cf = scal->viscosity;

	INTERPOLATE(fs->DA_CEN, outbuf->lbcen, InterpCenterCorner, GET_VISCOSITY)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteVelocity"
PetscErrorCode PVOutWriteVelocity(JacRes *jr, OutBuf *outbuf)
{
	ACCESS_FUNCTION_HEADER

	cf = scal->velocity;
	iflag.use_bound = PETSC_TRUE;

	// x-velocity
	ierr = InterpXFaceCorner(outbuf->fs, jr->lvx, outbuf->lbcor, iflag); CHKERRQ(ierr);
	ierr = OutBufPut3DVecComp(outbuf, 3, 0, cf, 0.0); CHKERRQ(ierr);

	// y-velocity
	ierr = InterpYFaceCorner(outbuf->fs, jr->lvy, outbuf->lbcor, iflag); CHKERRQ(ierr);
	ierr = OutBufPut3DVecComp(outbuf, 3, 1, cf, 0.0); CHKERRQ(ierr);

	// z-velocity
	ierr = InterpZFaceCorner(outbuf->fs, jr->lvz, outbuf->lbcor, iflag); CHKERRQ(ierr);
	ierr = OutBufPut3DVecComp(outbuf, 3, 2, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWritePressure"
PetscErrorCode PVOutWritePressure(JacRes *jr, OutBuf *outbuf)
{
	ACCESS_FUNCTION_HEADER

	cf = scal->stress;
	iflag.use_bound = PETSC_TRUE;

	ierr = InterpCenterCorner(outbuf->fs, jr->lp, outbuf->lbcor, iflag); CHKERRQ(ierr);

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteTemperature"
PetscErrorCode PVOutWriteTemperature(JacRes *jr, OutBuf *outbuf)
{
	ACCESS_FUNCTION_HEADER

	cf = scal->temperature;
	iflag.use_bound = PETSC_TRUE;

	ierr = InterpCenterCorner(outbuf->fs, jr->lT, outbuf->lbcor, iflag); CHKERRQ(ierr);

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, scal->Tshift); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteDevStress"
PetscErrorCode PVOutWriteDevStress(JacRes *jr, OutBuf *outbuf)
{
	// NOTE! See warning about component ordering scheme above

	INTERPOLATION_FUNCTION_HEADER

	// macros to copy deviatoric stress components to buffer
	#define GET_SXX buff[k][j][i] = jr->svCell[iter++].sxx;
	#define GET_SYY buff[k][j][i] = jr->svCell[iter++].syy;
	#define GET_SZZ buff[k][j][i] = jr->svCell[iter++].szz;
	#define GET_SXY buff[k][j][i] = jr->svXYEdge[iter++].s;
	#define GET_SXZ buff[k][j][i] = jr->svXZEdge[iter++].s;
	#define GET_SYZ buff[k][j][i] = jr->svYZEdge[iter++].s;

	cf = scal->stress;

	INTERPOLATE(fs->DA_CEN, outbuf->lbcen, InterpCenterCorner, GET_SXX)
	ierr = OutBufPut3DVecComp(outbuf, 6, 0, cf, 0.0); CHKERRQ(ierr);

	INTERPOLATE(fs->DA_CEN, outbuf->lbcen, InterpCenterCorner, GET_SYY)
	ierr = OutBufPut3DVecComp(outbuf, 6, 1, cf, 0.0); CHKERRQ(ierr);

	INTERPOLATE(fs->DA_CEN, outbuf->lbcen, InterpCenterCorner, GET_SZZ)
	ierr = OutBufPut3DVecComp(outbuf, 6, 2, cf, 0.0); CHKERRQ(ierr);

	INTERPOLATE(fs->DA_XY, outbuf->lbxy, InterpXYEdgeCorner, GET_SXY)
	ierr = OutBufPut3DVecComp(outbuf, 6, 3, cf, 0.0); CHKERRQ(ierr);

	INTERPOLATE(fs->DA_YZ, outbuf->lbyz, InterpYZEdgeCorner, GET_SYZ)
	ierr = OutBufPut3DVecComp(outbuf, 6, 4, cf, 0.0); CHKERRQ(ierr);

	INTERPOLATE(fs->DA_XZ, outbuf->lbxz, InterpXZEdgeCorner, GET_SXZ)
	ierr = OutBufPut3DVecComp(outbuf, 6, 5, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteJ2DevStress"
PetscErrorCode PVOutWriteJ2DevStress(JacRes *jr, OutBuf *outbuf)
{
	SolVarDev *svDev;

	INTERPOLATION_FUNCTION_HEADER

	// macro to copy deviatoric stress invariant to buffer
	#define _GET_J2_DEV_STRESS_ \
		svDev = &jr->svCell[iter++].svDev; \
		buff[k][j][i] = 2.0*svDev->eta*svDev->DII;

	cf = scal->stress;

//	INTERPOLATE_CENTER(_GET_J2_DEV_STRESS_)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteStrainRate"
PetscErrorCode PVOutWriteStrainRate(JacRes *jr, OutBuf *outbuf)
{
	// NOTE! See warning about component ordering scheme above

	INTERPOLATION_FUNCTION_HEADER

	// macros to copy deviatoric strain rate components to buffer
	#define _GET_DXX_ buff[k][j][i] = jr->svCell[iter++].dxx;
	#define _GET_DYY_ buff[k][j][i] = jr->svCell[iter++].dyy;
	#define _GET_DZZ_ buff[k][j][i] = jr->svCell[iter++].dzz;
	#define _GET_DXY_ buff[k][j][i] = jr->svXYEdge[iter++].d;
	#define _GET_DXZ_ buff[k][j][i] = jr->svXZEdge[iter++].d;
	#define _GET_DYZ_ buff[k][j][i] = jr->svYZEdge[iter++].d;

	cf = scal->strain_rate;

//	INTERPOLATE_CENTER(_GET_DXX_)

	ierr = OutBufPut3DVecComp(outbuf, 6, 0, cf, 0.0); CHKERRQ(ierr);

//	INTERPOLATE_CENTER(_GET_DYY_)

	ierr = OutBufPut3DVecComp(outbuf, 6, 1, cf, 0.0); CHKERRQ(ierr);

//	INTERPOLATE_CENTER(_GET_DZZ_)

	ierr = OutBufPut3DVecComp(outbuf, 6, 2, cf, 0.0); CHKERRQ(ierr);

//	INTERPOLATE_XY_EDGE(_GET_DXY_)

	ierr = OutBufPut3DVecComp(outbuf, 6, 3, cf, 0.0); CHKERRQ(ierr);

//	INTERPOLATE_YZ_EDGE(_GET_DYZ_)

	ierr = OutBufPut3DVecComp(outbuf, 6, 4, cf, 0.0); CHKERRQ(ierr);

//	INTERPOLATE_XZ_EDGE(_GET_DXZ_)

	ierr = OutBufPut3DVecComp(outbuf, 6, 5, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteJ2StrainRate"
PetscErrorCode PVOutWriteJ2StrainRate(JacRes *jr, OutBuf *outbuf)
{

	SolVarCell *svCell;
	PetscScalar d, J2;

	INTERPOLATION_FUNCTION_HEADER

	// macros to copy deviatoric strain rate invariant to buffer
	#define _GET_J2_STRAIN_RATE_CENTER_ \
		svCell = &jr->svCell[iter++]; \
		d = svCell->dxx; J2  = d*d; \
		d = svCell->dyy; J2 += d*d; \
		d = svCell->dzz; J2 += d*d; \
		buff[k][j][i] = 0.5*J2;

	#define _GET_J2_STRAIN_RATE_XY_EDGE_ \
		d = jr->svXYEdge[iter++].d; \
		buff[k][j][i] = d*d;

	#define _GET_J2_STRAIN_RATE_XZ_EDGE_ \
		d = jr->svXZEdge[iter++].d; \
		buff[k][j][i] = d*d;

	#define _GET_J2_STRAIN_RATE_YZ_EDGE_ \
		d = jr->svYZEdge[iter++].d; \
		buff[k][j][i] = d*d;

	cf = scal->strain_rate;

	iflag.update = PETSC_TRUE;

	ierr = VecSet(outbuf->lbcor, 0.0);

// WARNING RHIS MUST BE CHANGED

//	INTERPOLATE_CENTER(_GET_J2_STRAIN_RATE_CENTER_)

//	INTERPOLATE_XY_EDGE(_GET_J2_STRAIN_RATE_XY_EDGE_)

//	INTERPOLATE_XZ_EDGE(_GET_J2_STRAIN_RATE_XZ_EDGE_)

//	INTERPOLATE_YZ_EDGE(_GET_J2_STRAIN_RATE_YZ_EDGE_)

	// compute & store second invariant
//	ierr = VecSqrtAbs(outbuf->lbcor); CHKERRQ(ierr);

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteVolRate"
PetscErrorCode PVOutWriteVolRate(JacRes *jr, OutBuf *outbuf)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = 0; CHKERRQ(ierr);
	if(jr)     jr = NULL;
	if(outbuf) outbuf = NULL;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteVorticity"
PetscErrorCode PVOutWriteVorticity(JacRes *jr, OutBuf *outbuf)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = 0; CHKERRQ(ierr);
	if(jr)  jr = NULL;
	if(outbuf) outbuf = NULL;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteAngVelMag"
PetscErrorCode PVOutWriteAngVelMag(JacRes *jr, OutBuf *outbuf)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = 0; CHKERRQ(ierr);
	if(jr)  jr = NULL;
	if(outbuf) outbuf = NULL;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteTotStrain"
PetscErrorCode PVOutWriteTotStrain(JacRes *jr, OutBuf *outbuf)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = 0; CHKERRQ(ierr);
	if(jr)  jr = NULL;
	if(outbuf) outbuf = NULL;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWritePlastStrain"
PetscErrorCode PVOutWritePlastStrain(JacRes *jr, OutBuf *outbuf)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = 0; CHKERRQ(ierr);
	if(jr)  jr = NULL;
	if(outbuf) outbuf = NULL;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWritePlastDissip"
PetscErrorCode PVOutWritePlastDissip(JacRes *jr, OutBuf *outbuf)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = 0; CHKERRQ(ierr);
	if(jr)  jr = NULL;
	if(outbuf) outbuf = NULL;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteTotDispl"
PetscErrorCode PVOutWriteTotDispl(JacRes *jr, OutBuf *outbuf)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = 0; CHKERRQ(ierr);
	if(jr)  jr = NULL;
	if(outbuf) outbuf = NULL;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
// DEBUG VECTORS
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteMomentRes"
PetscErrorCode PVOutWriteMomentRes(JacRes *jr, OutBuf *outbuf)
{
	ACCESS_FUNCTION_HEADER

	cf = scal->force;

	// x-residual
	GLOBAL_TO_LOCAL(outbuf->fs->DA_X, jr->gfx, jr->lfx)

	ierr = InterpXFaceCorner(outbuf->fs, jr->lfx, outbuf->lbcor, iflag); CHKERRQ(ierr);
	ierr = OutBufPut3DVecComp(outbuf, 3, 0, cf, 0.0); CHKERRQ(ierr);

	// y-residual
	GLOBAL_TO_LOCAL(outbuf->fs->DA_Y, jr->gfy, jr->lfy)

	ierr = InterpYFaceCorner(outbuf->fs, jr->lfy, outbuf->lbcor, iflag); CHKERRQ(ierr);
	ierr = OutBufPut3DVecComp(outbuf, 3, 1, cf, 0.0); CHKERRQ(ierr);

	// z-residual
	GLOBAL_TO_LOCAL(outbuf->fs->DA_Z, jr->gfz, jr->lfz)

	ierr = InterpZFaceCorner(outbuf->fs, jr->lfz, outbuf->lbcor, iflag); CHKERRQ(ierr);
	ierr = OutBufPut3DVecComp(outbuf, 3, 2, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteContRes"
PetscErrorCode PVOutWriteContRes(JacRes *jr, OutBuf *outbuf)
{
	ACCESS_FUNCTION_HEADER

	cf  = scal->strain_rate;

	// scatter to local vector
	GLOBAL_TO_LOCAL(outbuf->fs->DA_CEN, jr->gc, outbuf->lbcen)

	ierr = InterpCenterCorner(outbuf->fs, outbuf->lbcen, outbuf->lbcor, iflag); CHKERRQ(ierr);

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWritEnergRes"
PetscErrorCode PVOutWritEnergRes(JacRes *jr, OutBuf *outbuf)
{
	ACCESS_FUNCTION_HEADER

	cf = scal->dissipation_rate;

	// scatter to local vector
	GLOBAL_TO_LOCAL(outbuf->fs->DA_CEN, jr->ge, outbuf->lbcen)

	ierr = InterpCenterCorner(outbuf->fs, outbuf->lbcen, outbuf->lbcor, iflag); CHKERRQ(ierr);

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteDII_CEN"
PetscErrorCode PVOutWriteDII_CEN(JacRes *jr, OutBuf *outbuf)
{
	INTERPOLATION_FUNCTION_HEADER

	// macros to copy effective strain rate invariant to buffer
	#define _GET_DII_CENTER_ \
		buff[k][j][i] = jr->svCell[iter++].svDev.DII;

	cf  = scal->strain_rate;

//	INTERPOLATE_CENTER(_GET_DII_CENTER_)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteDII_XY"
PetscErrorCode PVOutWriteDII_XY(JacRes *jr, OutBuf *outbuf)
{
	INTERPOLATION_FUNCTION_HEADER

	// macros to copy effective strain rate invariant to buffer
	#define _GET_DII_XY_EDGE_ \
		buff[k][j][i] = jr->svXYEdge[iter++].svDev.DII;

	cf = scal->strain_rate;

//	INTERPOLATE_XY_EDGE(_GET_DII_XY_EDGE_)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteDII_XZ"
PetscErrorCode PVOutWriteDII_XZ(JacRes *jr, OutBuf *outbuf)
{
	INTERPOLATION_FUNCTION_HEADER

	// macros to copy effective strain rate invariant to buffer
	#define _GET_DII_XZ_EDGE_ \
		buff[k][j][i] = jr->svXZEdge[iter++].svDev.DII;

	cf = scal->strain_rate;

//	INTERPOLATE_XZ_EDGE(_GET_DII_XZ_EDGE_)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "PVOutWriteDII_YZ"
PetscErrorCode PVOutWriteDII_YZ(JacRes *jr, OutBuf *outbuf)
{
	INTERPOLATION_FUNCTION_HEADER

	// macros to copy effective strain rate invariant to buffer
	#define _GET_DII_YZ_EDGE_ \
		buff[k][j][i] = jr->svYZEdge[iter++].svDev.DII;

	cf = scal->strain_rate;

//	INTERPOLATE_YZ_EDGE(_GET_DII_YZ_EDGE_)

	ierr = OutBufPut3DVecComp(outbuf, 1, 0, cf, 0.0); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
