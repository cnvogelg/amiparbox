#define __NOLIBBASE__
#include <proto/exec.h>
#include <proto/misc.h>
#include <proto/cia.h>

#include <exec/interrupts.h>
#include <resources/misc.h>
#include <resources/cia.h>
#include <hardware/cia.h>

#include "autoconf.h"
#include "debug.h"
#include "pario.h"
#include "compiler.h"

extern struct CIA ciaa, ciab;

struct pario_handle {
  /* Note: the first three fields are used from assembler irq handler! */
  struct Library *sysBase; /* +0: sysBase */
  struct Task *sigTask;    /* +4: sigTask */
  ULONG sigMask;           /* +8: sigMask */

  ULONG initFlags;
  struct Library *miscBase;
  struct Library *ciaABase;
  struct Interrupt ackIrq;

  struct pario_port port;
};

#define MiscBase ph->miscBase
#define CIAABase ph->ciaABase

const char *pario_tag = "pario";

static void setup_port(struct pario_handle *ph)
{
  struct pario_port *p = &ph->port;

  /* CIA A - Port B = Parallel Port */
  p->data_port = &ciaa.ciaprb;
  p->data_ddr  = &ciaa.ciaddrb;

  /* CIA B - Port A = Control Lines */
  p->ctrl_port = &ciab.ciapra;
  p->ctrl_ddr  = &ciab.ciaddra;

  p->busy_bit  = CIAB_PRTRBUSY;
  p->pout_bit  = CIAB_PRTRPOUT;
  p->sel_bit   = CIAB_PRTRSEL;

  p->busy_mask = CIAF_PRTRBUSY;
  p->pout_mask = CIAF_PRTRPOUT;
  p->sel_mask  = CIAF_PRTRSEL;
}

struct pario_handle *pario_init(struct Library *SysBase)
{
  /* alloc handle */
  struct pario_handle *ph;
  ph = AllocMem(sizeof(struct pario_handle), MEMF_CLEAR);
  if(ph == NULL) {
    return NULL;
  }
  ph->sysBase = SysBase;

  /* get misc.resource */
  D(("OpenResouce(MISCNAME)\n"));
  MiscBase = OpenResource(MISCNAME);
  if(MiscBase != NULL) {

    /* get ciaa.resource */
    D(("OpenResource(CIANAME)\n"));
    CIAABase = OpenResource(CIAANAME);
    if(CIAABase != NULL) {

      /* obtain exclusive access to the parallel hardware */
      if (!AllocMiscResource(MR_PARALLELPORT, pario_tag)) {
        ph->initFlags = 1;
        if (!AllocMiscResource(MR_PARALLELBITS, pario_tag)) {
          ph->initFlags = 3;

          setup_port(ph);

          /* ok */
          return ph;
        }
      }
    }
  }

  /* something failed. clean up */
  pario_exit(ph);
  return NULL;
}

#define SysBase ph->sysBase

void pario_exit(struct pario_handle *ph)
{
  if(ph == NULL) {
    return;
  }

  /* free resources */
  if(ph->initFlags & 1) {
    FreeMiscResource(MR_PARALLELPORT);
  }
  if(ph->initFlags & 2) {
    FreeMiscResource(MR_PARALLELBITS);
  }

  /* free handle */
  FreeMem(ph, sizeof(struct pario_handle));
}

struct pario_port *pario_get_port(struct pario_handle *ph)
{
  return &ph->port;
}

extern void ASM pario_irq_handler(REG(a1, struct pario_handle *ph));

int pario_setup_ack_irq(struct pario_handle *ph, struct Task *sigTask, BYTE signal)
{
  int error = 0;

  ph->ackIrq.is_Node.ln_Type = NT_INTERRUPT;
  ph->ackIrq.is_Node.ln_Pri  = 127;
  ph->ackIrq.is_Node.ln_Name = (char *)pario_tag;
  ph->ackIrq.is_Data         = (APTR)ph;
  ph->ackIrq.is_Code         = pario_irq_handler;

  ph->sigTask = sigTask;
  ph->sigMask = 1 << signal;

  Disable();
  if (!AddICRVector(CIAABase, CIAICRB_FLG, &ph->ackIrq)) {

    D(("ack_irq_handler @%08lx  task @%08lx  sigmask %08lx\n",
       &pario_irq_handler, ph->sigTask, ph->sigMask));

    /* disable pending irqs first */
    AbleICR(CIAABase, CIAICRF_FLG);
  } else {
    error = 1;
  }
  Enable();

  if(!error) {
    /* clea irq flag */
    SetICR(CIAABase, CIAICRF_FLG);
    /* enable irq */
    AbleICR(CIAABase, CIAICRF_FLG | CIAICRF_SETCLR);

    ph->initFlags |= 4;
  }

  return error;
}

void pario_cleanup_ack_irq(struct pario_handle *ph)
{
  if(ph->initFlags & 4 == 0) {
    return;
  }

  /* disable pending irqs first */
  AbleICR(CIAABase, CIAICRF_FLG);
  /* clea irq flag */
  SetICR(CIAABase, CIAICRF_FLG);
  /* remove vector */
  RemICRVector(CIAABase, CIAICRB_FLG, &ph->ackIrq);

  ph->initFlags &= ~4;
}
