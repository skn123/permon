#include <permon/private/permonpcimpl.h>
#include <petscmat.h>

const char *PCDualTypes[] = {"none", "lumped", "PCDualType", "PC_DUAL_", 0};

PetscLogEvent PC_Dual_Apply, PC_Dual_MatMultSchur;

/* Private context (data structure) for the Dual preconditioner. */
typedef struct {
  PetscBool  setfromoptionscalled;
  PCDualType pcdualtype;
  Mat        C_bb, At;
  Vec        xwork, ywork;
} PC_Dual;

#undef __FUNCT__
#define __FUNCT__ "PCDualSetType_Dual"
static PetscErrorCode PCDualSetType_Dual(PC pc, PCDualType type)
{
  PC_Dual *data = (PC_Dual *)pc->data;

  PetscFunctionBegin;
  data->pcdualtype = type;
  PetscCall(PCReset(pc));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCDualSetType"
PetscErrorCode PCDualSetType(PC pc, PCDualType type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc, PC_CLASSID, 1);
  PetscValidLogicalCollectiveEnum(pc, type, 2);
  PetscTryMethod(pc, "PCDualSetType_Dual_C", (PC, PCDualType), (pc, type));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCDualGetType_Dual"
static PetscErrorCode PCDualGetType_Dual(PC pc, PCDualType *type)
{
  PC_Dual *data = (PC_Dual *)pc->data;

  PetscFunctionBegin;
  *type = data->pcdualtype;
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCDualGetType"
PetscErrorCode PCDualGetType(PC pc, PCDualType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc, PC_CLASSID, 1);
  PetscAssertPointer(type, 2);
  PetscTryMethod(pc, "PCDualGetType_Dual_C", (PC, PCDualType *), (pc, type));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCApply_Dual"
static PetscErrorCode PCApply_Dual(PC pc, Vec x, Vec y)
{
  PC_Dual *ctx = (PC_Dual *)pc->data;

  PetscFunctionBegin;
  PetscCall(PetscLogEventBegin(PC_Dual_Apply, pc, x, y, 0));
  PetscCall(MatMult(ctx->At, x, ctx->xwork));

  PetscCall(PetscLogEventBegin(PC_Dual_MatMultSchur, pc, x, y, 0));
  PetscCall(MatMult(ctx->C_bb, ctx->xwork, ctx->ywork));
  PetscCall(PetscLogEventEnd(PC_Dual_MatMultSchur, pc, x, y, 0));

  PetscCall(MatMultTranspose(ctx->At, ctx->ywork, y));
  PetscCall(PetscLogEventEnd(PC_Dual_Apply, pc, x, y, 0));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCApply_Dual_None"
static PetscErrorCode PCApply_Dual_None(PC pc, Vec x, Vec y)
{
  PetscFunctionBegin;
  PetscCall(VecCopy(x, y));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCSetUp_Dual"
static PetscErrorCode PCSetUp_Dual(PC pc)
{
  PC_Dual *ctx = (PC_Dual *)pc->data;
  Mat      F   = pc->mat;
  Mat      Bt, K;

  PetscFunctionBegin;
  PetscCall(PetscInfo(pc, "using PCDualType %s\n", PCDualTypes[ctx->pcdualtype]));

  if (ctx->pcdualtype == PC_DUAL_NONE) {
    pc->ops->apply = PCApply_Dual_None;
    PetscFunctionReturn(PETSC_SUCCESS);
  }

  pc->ops->apply = PCApply_Dual;

  PetscCall(PetscObjectQuery((PetscObject)F, "Bt", (PetscObject *)&Bt));
  PetscCall(PetscObjectQuery((PetscObject)F, "K", (PetscObject *)&K));

  if (ctx->pcdualtype == PC_DUAL_LUMPED) {
    ctx->At = Bt;
    PetscCall(PetscObjectReference((PetscObject)Bt));
    ctx->C_bb = K;
    PetscCall(PetscObjectReference((PetscObject)K));
    PetscCall(MatCreateVecs(ctx->C_bb, &ctx->xwork, &ctx->ywork));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCReset_Dual"
static PetscErrorCode PCReset_Dual(PC pc)
{
  PC_Dual *ctx = (PC_Dual *)pc->data;

  PetscFunctionBegin;
  PetscCall(MatDestroy(&ctx->At));
  PetscCall(MatDestroy(&ctx->C_bb));
  PetscCall(VecDestroy(&ctx->xwork));
  PetscCall(VecDestroy(&ctx->ywork));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCView_Dual"
static PetscErrorCode PCView_Dual(PC pc, PetscViewer viewer)
{
  PC_Dual  *ctx = (PC_Dual *)pc->data;
  PetscBool iascii;

  PetscFunctionBegin;
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (!iascii) PetscFunctionReturn(PETSC_SUCCESS);
  PetscCall(PetscViewerASCIIPrintf(viewer, "  PCDualType: %d (%s)\n", ctx->pcdualtype, PCDualTypes[ctx->pcdualtype]));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCDestroy_Dual"
static PetscErrorCode PCDestroy_Dual(PC pc)
{
  //PC_Dual         *ctx = (PC_Dual*)pc->data;

  PetscFunctionBegin;
  PetscCall(PCReset_Dual(pc));
  PetscCall(PetscFree(pc->data));
  PetscCall(PetscObjectComposeFunction((PetscObject)pc, "PCDualSetType_Dual_C", NULL));
  PetscCall(PetscObjectComposeFunction((PetscObject)pc, "PCDualGetType_Dual_C", NULL));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCSetFromOptions_Dual"
PetscErrorCode PCSetFromOptions_Dual(PC pc, PetscOptionItems PetscOptionsObject)
{
  PC_Dual *ctx = (PC_Dual *)pc->data;

  PetscFunctionBegin;
  PetscOptionsHeadBegin(PetscOptionsObject, "PCDUAL options");
  PetscCall(PetscOptionsEnum("-pc_dual_type", "PCDUAL type", "PCDualSetType", PCDualTypes, (PetscEnum)ctx->pcdualtype, (PetscEnum *)&ctx->pcdualtype, NULL));
  PetscOptionsHeadEnd();
  ctx->setfromoptionscalled = PETSC_TRUE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef __FUNCT__
#define __FUNCT__ "PCCreate_Dual"
PERMON_EXTERN PetscErrorCode PCCreate_Dual(PC pc)
{
  PC_Dual         *ctx;
  static PetscBool registered = PETSC_FALSE;

  PetscFunctionBegin;
  /* Create the private data structure for this preconditioner and
     attach it to the PC object.  */
  PetscCall(PetscNew(&ctx));
  pc->data = (void *)ctx;

  ctx->setfromoptionscalled = PETSC_FALSE;
  ctx->pcdualtype           = PC_DUAL_NONE;

  /* set general PC functions already implemented for this PC type */
  pc->ops->apply          = PCApply_Dual;
  pc->ops->destroy        = PCDestroy_Dual;
  pc->ops->reset          = PCReset_Dual;
  pc->ops->setfromoptions = PCSetFromOptions_Dual;
  pc->ops->setup          = PCSetUp_Dual;
  pc->ops->view           = PCView_Dual;

  /* set type-specific functions */
  PetscCall(PetscObjectComposeFunction((PetscObject)pc, "PCDualSetType_Dual_C", PCDualSetType_Dual));
  PetscCall(PetscObjectComposeFunction((PetscObject)pc, "PCDualGetType_Dual_C", PCDualGetType_Dual));

  /* prepare log events*/
  if (!registered) {
    PetscCall(PetscLogEventRegister("PCdualApply", PC_CLASSID, &PC_Dual_Apply));
    PetscCall(PetscLogEventRegister("PCdualApplySchur", PC_CLASSID, &PC_Dual_MatMultSchur));
    registered = PETSC_TRUE;
  }

  /* initialize inner data */
  PetscFunctionReturn(PETSC_SUCCESS);
}
