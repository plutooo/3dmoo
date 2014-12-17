#ifndef __VFPDOUBLE_H_
#define __VFPDOUBLE_H_

u32 vfp_double_add(struct vfp_double *vdd, struct vfp_double *vdn, struct vfp_double *vdm, u32 fpscr);
u32 vfp_double_normaliseroundintern(ARMul_State* state, struct vfp_double *vd, u32 fpscr, u32 exceptions, const char *func);
u32 vfp_double_multiply(struct vfp_double *vdd, struct vfp_double *vdn, struct vfp_double *vdm, u32 fpscr);
u32 vfp_double_add(struct vfp_double *vdd, struct vfp_double *vdn, struct vfp_double *vdm, u32 fpscr);
u32 vfp_double_fcvtsinterncutting(ARMul_State* state, int sd, struct vfp_double* dm, u32 fpscr); //ichfly for internal use only


#endif // __VFPDOUBLE_H_