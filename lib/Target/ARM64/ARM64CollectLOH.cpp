//===-------------- ARM64CollectLOH.cpp - ARM64 collect LOH pass --*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that collect the Linker Optimization Hint (LOH).
// This pass should be run at the very end of the compilation flow, just before
// assembly printer.
// To be useful for the linker, the LOH must be printed into the assembly file.
// Currently supported LOH are:
// * So called non-ADRP-related:
//   - .loh AdrpAddLdr L1, L2, L3:
//     L1: adrp xA, sym@PAGE
//     L2: add xB, xA, sym@PAGEOFF
//     L3: ldr xC, [xB, #imm]
//   - .loh AdrpLdrGotLdr L1, L2, L3:
//     L1: adrp xA, sym@GOTPAGE
//     L2: ldr xB, [xA, sym@GOTPAGEOFF]
//     L3: ldr xC, [xB, #imm]
//   - .loh AdrpLdr L1, L3:
//     L1: adrp xA, sym@PAGE
//     L3: ldr xC, [xA, sym@PAGEOFF]
//   - .loh AdrpAddStr L1, L2, L3:
//     L1: adrp xA, sym@PAGE
//     L2: add xB, xA, sym@PAGEOFF
//     L3: str xC, [xB, #imm]
//   - .loh AdrpLdrGotStr L1, L2, L3:
//     L1: adrp xA, sym@GOTPAGE
//     L2: ldr xB, [xA, sym@GOTPAGEOFF]
//     L3: str xC, [xB, #imm]
//   - .loh AdrpAdd L1, L2:
//     L1: adrp xA, sym@PAGE
//     L2: add xB, xA, sym@PAGEOFF
//   For all these LOHs, L1, L2, L3 form a simple chain:
//   L1 result is used only by L2 and L2 result by L3.
//   L3 LOH-related argument is defined only by L2 and L2 LOH-related argument
//   by L1.
//
// * So called ADRP-related:
//  - .loh AdrpAdrp L2, L1:
//    L2: ADRP xA, sym1@PAGE
//    L1: ADRP xA, sym2@PAGE
//    L2 dominates L1 and xA is not redifined between L2 and L1
//
// More information are available in the design document attached to
// rdar://11956674
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "arm64-collect-loh"
#include "ARM64.h"
#include "ARM64InstrInfo.h"
#include "ARM64MachineFunctionInfo.h"
#include "MCTargetDesc/ARM64AddressingModes.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
using namespace llvm;

static cl::opt<bool>
PreCollectRegister("arm64-collect-loh-pre-collect-register", cl::Hidden,
                   cl::desc("Restrict analysis to registers invovled"
                            " in LOHs"),
                   cl::init(true));

static cl::opt<bool>
BasicBlockScopeOnly("arm64-collect-loh-bb-only", cl::Hidden,
                    cl::desc("Restrict analysis at basic block scope"),
                    cl::init(true));

STATISTIC(NumADRPSimpleCandidate,
          "Number of simplifiable ADRP dominate by another");
STATISTIC(NumADRPComplexCandidate2,
          "Number of simplifiable ADRP reachable by 2 defs");
STATISTIC(NumADRPComplexCandidate3,
          "Number of simplifiable ADRP reachable by 3 defs");
STATISTIC(NumADRPComplexCandidateOther,
          "Number of simplifiable ADRP reachable by 4 or more defs");
STATISTIC(NumADDToSTRWithImm,
          "Number of simplifiable STR with imm reachable by ADD");
STATISTIC(NumLDRToSTRWithImm,
          "Number of simplifiable STR with imm reachable by LDR");
STATISTIC(NumADDToSTR, "Number of simplifiable STR reachable by ADD");
STATISTIC(NumLDRToSTR, "Number of simplifiable STR reachable by LDR");
STATISTIC(NumADDToLDRWithImm,
          "Number of simplifiable LDR with imm reachable by ADD");
STATISTIC(NumLDRToLDRWithImm,
          "Number of simplifiable LDR with imm reachable by LDR");
STATISTIC(NumADDToLDR, "Number of simplifiable LDR reachable by ADD");
STATISTIC(NumLDRToLDR, "Number of simplifiable LDR reachable by LDR");
STATISTIC(NumADRPToLDR, "Number of simplifiable LDR reachable by ADRP");
STATISTIC(NumCplxLvl1, "Number of complex case of level 1");
STATISTIC(NumTooCplxLvl1, "Number of too complex case of level 1");
STATISTIC(NumCplxLvl2, "Number of complex case of level 2");
STATISTIC(NumTooCplxLvl2, "Number of too complex case of level 2");
STATISTIC(NumADRSimpleCandidate, "Number of simplifiable ADRP + ADD");
STATISTIC(NumADRComplexCandidate, "Number of too complex ADRP + ADD");

namespace llvm {
void initializeARM64CollectLOHPass(PassRegistry &);
}

namespace {
struct ARM64CollectLOH : public MachineFunctionPass {
  static char ID;
  ARM64CollectLOH() : MachineFunctionPass(ID) {
    initializeARM64CollectLOHPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnMachineFunction(MachineFunction &Fn);

  virtual const char *getPassName() const {
    return "ARM64 Collect Linker Optimization Hint (LOH)";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
    AU.addRequired<MachineDominatorTree>();
  }

private:
};

/// A set of MachineInstruction.
typedef SetVector<const MachineInstr *> SetOfMachineInstr;
/// Map a basic block to a set of instructions per register.
/// This is used to represent the exposed uses of a basic block
/// per register.
typedef MapVector<const MachineBasicBlock *, SetOfMachineInstr *>
BlockToSetOfInstrsPerColor;
/// Map a basic block to an instruction per register.
/// This is used to represent the live-out definitions of a basic block
/// per register.
typedef MapVector<const MachineBasicBlock *, const MachineInstr **>
BlockToInstrPerColor;
/// Map an instruction to a set of instructions. Used to represent the
/// mapping def to reachable uses or use to definitions.
typedef MapVector<const MachineInstr *, SetOfMachineInstr> InstrToInstrs;
/// Map a basic block to a BitVector.
/// This is used to record the kill registers per basic block.
typedef MapVector<const MachineBasicBlock *, BitVector> BlockToRegSet;

/// Map a register to a dense id.
typedef DenseMap<unsigned, unsigned> MapRegToId;
/// Map a dense id to a register. Used for debug purposes.
typedef SmallVector<unsigned, 32> MapIdToReg;
} // end anonymous namespace.

char ARM64CollectLOH::ID = 0;

INITIALIZE_PASS_BEGIN(ARM64CollectLOH, "arm64-collect-loh",
                      "ARM64 Collect Linker Optimization Hint (LOH)", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
INITIALIZE_PASS_END(ARM64CollectLOH, "arm64-collect-loh",
                    "ARM64 Collect Linker Optimization Hint (LOH)", false,
                    false)

/// Given a couple (MBB, reg) get the corresponding set of instruction from
/// the given "sets".
/// If this couple does not reference any set, an empty set is added to "sets"
/// for this couple and returned.
/// \param nbRegs is used internally allocate some memory. It must be consistent
/// with the way sets is used.
static SetOfMachineInstr &getSet(BlockToSetOfInstrsPerColor &sets,
                                 const MachineBasicBlock *MBB, unsigned reg,
                                 unsigned nbRegs) {
  SetOfMachineInstr *result;
  BlockToSetOfInstrsPerColor::iterator it = sets.find(MBB);
  if (it != sets.end()) {
    result = it->second;
  } else {
    result = sets[MBB] = new SetOfMachineInstr[nbRegs];
  }

  return result[reg];
}

/// Given a couple (reg, MI) get the corresponding set of instructions from the
/// the given "sets".
/// This is used to get the uses record in sets of a definition identified by
/// MI and reg, i.e., MI defines reg.
/// If the couple does not reference anything, an empty set is added to
/// "sets[reg]".
/// \pre set[reg] is valid.
static SetOfMachineInstr &getUses(InstrToInstrs *sets, unsigned reg,
                                  const MachineInstr *MI) {
  return sets[reg][MI];
}

/// Same as getUses but does not modify the input map: sets.
/// \return NULL if the couple (reg, MI) is not in sets.
static const SetOfMachineInstr *getUses(const InstrToInstrs *sets, unsigned reg,
                                        const MachineInstr *MI) {
  InstrToInstrs::const_iterator Res = sets[reg].find(MI);
  if (Res != sets[reg].end())
    return &(Res->second);
  return NULL;
}

/// Initialize the reaching definition algorithm:
/// For each basic block BB in MF, record:
/// - its kill set.
/// - its reachable uses (uses that are exposed to BB's predecessors).
/// - its the generated definitions.
/// \param DummyOp if not NULL, specifies a Dummy Operation to be added to
/// the list of uses of exposed defintions.
/// \param ADRPMode specifies to only consider ADRP instructions for generated
/// definition. It also consider definitions of ADRP instructions as uses and
/// ignore other uses. The ADRPMode is used to collect the information for LHO
/// that involve ADRP operation only.
static void initReachingDef(MachineFunction *MF,
                            InstrToInstrs *ColorOpToReachedUses,
                            BlockToInstrPerColor &Gen, BlockToRegSet &Kill,
                            BlockToSetOfInstrsPerColor &ReachableUses,
                            const MapRegToId &RegToId,
                            const MachineInstr *DummyOp, bool ADRPMode) {
  const TargetMachine &TM = MF->getTarget();
  const TargetRegisterInfo *TRI = TM.getRegisterInfo();

  unsigned NbReg = RegToId.size();

  for (MachineFunction::const_iterator IMBB = MF->begin(), IMBBEnd = MF->end();
       IMBB != IMBBEnd; ++IMBB) {
    const MachineBasicBlock *MBB = &(*IMBB);
    const MachineInstr **&BBGen = Gen[MBB];
    BBGen = new const MachineInstr *[NbReg];
    memset(BBGen, 0, sizeof(const MachineInstr *) * NbReg);

    BitVector &BBKillSet = Kill[MBB];
    BBKillSet.resize(NbReg);
    for (MachineBasicBlock::const_iterator II = MBB->begin(), IEnd = MBB->end();
         II != IEnd; ++II) {
      bool IsADRP = II->getOpcode() == ARM64::ADRP;

      // Process uses first.
      if (IsADRP || !ADRPMode)
        for (MachineInstr::const_mop_iterator IO = II->operands_begin(),
                                              IOEnd = II->operands_end();
             IO != IOEnd; ++IO) {
          // Treat ADRP def as use, as the goal of the analysis is to find
          // ADRP defs reached by other ADRP defs.
          if (!IO->isReg() || (!ADRPMode && !IO->isUse()) ||
              (ADRPMode && (!IsADRP || !IO->isDef())))
            continue;
          unsigned CurReg = IO->getReg();
          MapRegToId::const_iterator ItCurRegId = RegToId.find(CurReg);
          if (ItCurRegId == RegToId.end())
            continue;
          CurReg = ItCurRegId->second;

          // if CurReg has not been defined, this use is reachable.
          if (!BBGen[CurReg] && !BBKillSet.test(CurReg))
            getSet(ReachableUses, MBB, CurReg, NbReg).insert(&(*II));
          // current basic block definition for this color, if any, is in Gen.
          if (BBGen[CurReg])
            getUses(ColorOpToReachedUses, CurReg, BBGen[CurReg]).insert(&(*II));
        }

      // Process clobbers.
      for (MachineInstr::const_mop_iterator IO = II->operands_begin(),
                                            IOEnd = II->operands_end();
           IO != IOEnd; ++IO) {
        if (!IO->isRegMask())
          continue;
        // Clobbers kill the related colors.
        const uint32_t *PreservedRegs = IO->getRegMask();

        // Set generated regs.
        for (MapRegToId::const_iterator ItRegId = RegToId.begin(),
                                        EndIt = RegToId.end();
             ItRegId != EndIt; ++ItRegId) {
          unsigned Reg = ItRegId->second;
          // Use the global register ID when querying APIs external to this
          // pass.
          if (MachineOperand::clobbersPhysReg(PreservedRegs, ItRegId->first)) {
            // Do not register clobbered definition for no ADRP.
            // This definition is not used anyway (otherwise register
            // allocation is wrong).
            BBGen[Reg] = ADRPMode ? II : NULL;
            BBKillSet.set(Reg);
          }
        }
      }

      // Process defs
      for (MachineInstr::const_mop_iterator IO = II->operands_begin(),
                                            IOEnd = II->operands_end();
           IO != IOEnd; ++IO) {
        if (!IO->isReg() || !IO->isDef())
          continue;
        unsigned CurReg = IO->getReg();
        MapRegToId::const_iterator ItCurRegId = RegToId.find(CurReg);
        if (ItCurRegId == RegToId.end())
          continue;

        for (MCRegAliasIterator AI(CurReg, TRI, true); AI.isValid(); ++AI) {
          MapRegToId::const_iterator ItRegId = RegToId.find(*AI);
          assert(ItRegId != RegToId.end() &&
                 "Sub-register of an "
                 "involved register, not recorded as involved!");
          BBKillSet.set(ItRegId->second);
          BBGen[ItRegId->second] = &(*II);
        }
        BBGen[ItCurRegId->second] = &(*II);
      }
    }

    // If we restrict our analysis to basic block scope, conservatively add a
    // dummy
    // use for each generated value.
    if (!ADRPMode && DummyOp && !MBB->succ_empty())
      for (unsigned CurReg = 0; CurReg < NbReg; ++CurReg)
        if (BBGen[CurReg])
          getUses(ColorOpToReachedUses, CurReg, BBGen[CurReg]).insert(DummyOp);
  }
}

/// Reaching def core algorithm:
/// while an Out has changed
///    for each bb
///       for each color
///           In[bb][color] = U Out[bb.predecessors][color]
///           insert reachableUses[bb][color] in each in[bb][color]
///                 op.reachedUses
///
///           Out[bb] = Gen[bb] U (In[bb] - Kill[bb])
static void reachingDefAlgorithm(MachineFunction *MF,
                                 InstrToInstrs *ColorOpToReachedUses,
                                 BlockToSetOfInstrsPerColor &In,
                                 BlockToSetOfInstrsPerColor &Out,
                                 BlockToInstrPerColor &Gen, BlockToRegSet &Kill,
                                 BlockToSetOfInstrsPerColor &ReachableUses,
                                 unsigned NbReg) {
  bool HasChanged;
  do {
    HasChanged = false;
    for (MachineFunction::const_iterator IMBB = MF->begin(),
                                         IMBBEnd = MF->end();
         IMBB != IMBBEnd; ++IMBB) {
      const MachineBasicBlock *MBB = &(*IMBB);
      unsigned CurReg;
      for (CurReg = 0; CurReg < NbReg; ++CurReg) {
        SetOfMachineInstr &BBInSet = getSet(In, MBB, CurReg, NbReg);
        SetOfMachineInstr &BBReachableUses =
            getSet(ReachableUses, MBB, CurReg, NbReg);
        SetOfMachineInstr &BBOutSet = getSet(Out, MBB, CurReg, NbReg);
        unsigned Size = BBOutSet.size();
        //   In[bb][color] = U Out[bb.predecessors][color]
        for (MachineBasicBlock::const_pred_iterator
                 PredMBB = MBB->pred_begin(),
                 EndPredMBB = MBB->pred_end();
             PredMBB != EndPredMBB; ++PredMBB) {
          SetOfMachineInstr &PredOutSet = getSet(Out, *PredMBB, CurReg, NbReg);
          BBInSet.insert(PredOutSet.begin(), PredOutSet.end());
        }
        //   insert reachableUses[bb][color] in each in[bb][color] op.reachedses
        for (SetOfMachineInstr::const_iterator InstrIt = BBInSet.begin(),
                                               EndInstrIt = BBInSet.end();
             InstrIt != EndInstrIt; ++InstrIt) {
          SetOfMachineInstr &OpReachedUses =
              getUses(ColorOpToReachedUses, CurReg, *InstrIt);
          OpReachedUses.insert(BBReachableUses.begin(), BBReachableUses.end());
        }
        //           Out[bb] = Gen[bb] U (In[bb] - Kill[bb])
        if (!Kill[MBB].test(CurReg))
          BBOutSet.insert(BBInSet.begin(), BBInSet.end());
        if (Gen[MBB][CurReg])
          BBOutSet.insert(Gen[MBB][CurReg]);
        HasChanged |= BBOutSet.size() != Size;
      }
    }
  } while (HasChanged);
}

/// Release all memory dynamically allocated during the reaching
/// definition algorithm.
static void finitReachingDef(BlockToSetOfInstrsPerColor &In,
                             BlockToSetOfInstrsPerColor &Out,
                             BlockToInstrPerColor &Gen,
                             BlockToSetOfInstrsPerColor &ReachableUses) {
  for (BlockToSetOfInstrsPerColor::const_iterator IT = Out.begin(),
                                                  End = Out.end();
       IT != End; ++IT)
    delete[] IT->second;
  for (BlockToSetOfInstrsPerColor::const_iterator IT = In.begin(),
                                                  End = In.end();
       IT != End; ++IT)
    delete[] IT->second;
  for (BlockToSetOfInstrsPerColor::const_iterator IT = ReachableUses.begin(),
                                                  End = ReachableUses.end();
       IT != End; ++IT)
    delete[] IT->second;
  for (BlockToInstrPerColor::const_iterator IT = Gen.begin(), End = Gen.end();
       IT != End; ++IT)
    delete[] IT->second;
}

/// Reaching definiton algorithm.
/// \param MF function on which the algorithm will operate.
/// \param[out] ColorOpToReachedUses will contain the result of the reaching
/// def algorithm.
/// \param ADRPMode specify whether the reaching def algorithm should be tuned
/// for ADRP optimization. \see initReachingDef for more details.
/// \param DummyOp if not NULL, the algorithm will work at
/// basic block scope and will set for every exposed defintion a use to
/// @p DummyOp.
/// \pre ColorOpToReachedUses is an array of at least number of registers of
/// InstrToInstrs.
static void reachingDef(MachineFunction *MF,
                        InstrToInstrs *ColorOpToReachedUses,
                        const MapRegToId &RegToId, bool ADRPMode = false,
                        const MachineInstr *DummyOp = NULL) {
  // structures:
  // For each basic block.
  // Out: a set per color of definitions that reach the
  //      out boundary of this block.
  // In: Same as Out but for in boundary.
  // Gen: generated color in this block (one operation per color).
  // Kill: register set of killed color in this block.
  // ReachableUses: a set per color of uses (operation) reachable
  //                for "In" definitions.
  BlockToSetOfInstrsPerColor Out, In, ReachableUses;
  BlockToInstrPerColor Gen;
  BlockToRegSet Kill;

  // Initialize Gen, kill and reachableUses.
  initReachingDef(MF, ColorOpToReachedUses, Gen, Kill, ReachableUses, RegToId,
                  DummyOp, ADRPMode);

  // Algo.
  if (!DummyOp)
    reachingDefAlgorithm(MF, ColorOpToReachedUses, In, Out, Gen, Kill,
                         ReachableUses, RegToId.size());

  // finit.
  finitReachingDef(In, Out, Gen, ReachableUses);
}

#ifndef NDEBUG
/// print the result of the reaching definition algorithm.
static void printReachingDef(const InstrToInstrs *ColorOpToReachedUses,
                             unsigned NbReg, const TargetRegisterInfo *TRI,
                             const MapIdToReg &IdToReg) {
  unsigned CurReg;
  for (CurReg = 0; CurReg < NbReg; ++CurReg) {
    if (ColorOpToReachedUses[CurReg].empty())
      continue;
    DEBUG(dbgs() << "*** Reg " << PrintReg(IdToReg[CurReg], TRI) << " ***\n");

    InstrToInstrs::const_iterator DefsIt = ColorOpToReachedUses[CurReg].begin();
    InstrToInstrs::const_iterator DefsItEnd =
        ColorOpToReachedUses[CurReg].end();
    for (; DefsIt != DefsItEnd; ++DefsIt) {
      DEBUG(dbgs() << "Def:\n");
      DEBUG(DefsIt->first->print(dbgs()));
      DEBUG(dbgs() << "Reachable uses:\n");
      for (SetOfMachineInstr::const_iterator UsesIt = DefsIt->second.begin(),
                                             UsesItEnd = DefsIt->second.end();
           UsesIt != UsesItEnd; ++UsesIt) {
        DEBUG((*UsesIt)->print(dbgs()));
      }
    }
  }
}
#endif // NDEBUG

/// Answer the following question: Can Def be one of the definition
/// involved in a part of a LOH?
static bool canDefBePartOfLOH(const MachineInstr *Def) {
  unsigned Opc = Def->getOpcode();
  // Accept ADRP, ADDLow and LOADGot.
  switch (Opc) {
  default:
    return false;
  case ARM64::ADRP:
    return true;
  case ARM64::ADDXri:
    // Check immediate to see if the immediate is an address.
    switch (Def->getOperand(2).getType()) {
    default:
      return false;
    case MachineOperand::MO_GlobalAddress:
    case MachineOperand::MO_JumpTableIndex:
    case MachineOperand::MO_ConstantPoolIndex:
    case MachineOperand::MO_BlockAddress:
      return true;
    }
  case ARM64::LDRXui:
    // Check immediate to see if the immediate is an address.
    switch (Def->getOperand(2).getType()) {
    default:
      return false;
    case MachineOperand::MO_GlobalAddress:
      return true;
    }
  }
  // Unreachable.
  return false;
}

/// Check whether the given instruction can the end of a LOH chain involving a
/// store.
static bool isCandidateStore(const MachineInstr *Instr) {
  switch (Instr->getOpcode()) {
  default:
    return false;
  case ARM64::STRBui:
  case ARM64::STRHui:
  case ARM64::STRWui:
  case ARM64::STRXui:
  case ARM64::STRSui:
  case ARM64::STRDui:
  case ARM64::STRQui:
    // In case we have str xA, [xA, #imm], this is two different uses
    // of xA and we cannot fold, otherwise the xA stored may be wrong,
    // even if #imm == 0.
    if (Instr->getOperand(0).getReg() != Instr->getOperand(1).getReg())
      return true;
  }
  return false;
}

/// Given the result of a reaching defintion algorithm in ColorOpToReachedUses,
/// Build the Use to Defs information and filter out obvious non-LOH candidates.
/// In ADRPMode, non-LOH candidates are "uses" with non-ADRP definitions.
/// In non-ADRPMode, non-LOH candidates are "uses" with several definition,
/// i.e., no simple chain.
/// \param ADRPMode -- \see initReachingDef.
static void reachedUsesToDefs(InstrToInstrs &UseToReachingDefs,
                              const InstrToInstrs *ColorOpToReachedUses,
                              const MapRegToId &RegToId,
                              bool ADRPMode = false) {

  SetOfMachineInstr NotCandidate;
  unsigned NbReg = RegToId.size();
  MapRegToId::const_iterator EndIt = RegToId.end();
  for (unsigned CurReg = 0; CurReg < NbReg; ++CurReg) {
    // If this color is never defined, continue.
    if (ColorOpToReachedUses[CurReg].empty())
      continue;

    InstrToInstrs::const_iterator DefsIt = ColorOpToReachedUses[CurReg].begin();
    InstrToInstrs::const_iterator DefsItEnd =
        ColorOpToReachedUses[CurReg].end();
    for (; DefsIt != DefsItEnd; ++DefsIt) {
      for (SetOfMachineInstr::const_iterator UsesIt = DefsIt->second.begin(),
                                             UsesItEnd = DefsIt->second.end();
           UsesIt != UsesItEnd; ++UsesIt) {
        const MachineInstr *Def = DefsIt->first;
        MapRegToId::const_iterator It;
        // if all the reaching defs are not adrp, this use will not be
        // simplifiable.
        if ((ADRPMode && Def->getOpcode() != ARM64::ADRP) ||
            (!ADRPMode && !canDefBePartOfLOH(Def)) ||
            (!ADRPMode && isCandidateStore(*UsesIt) &&
             // store are LOH candidate iff the end of the chain is used as
             // base.
             ((It = RegToId.find((*UsesIt)->getOperand(1).getReg())) == EndIt ||
              It->second != CurReg))) {
          NotCandidate.insert(*UsesIt);
          continue;
        }
        // Do not consider self reaching as a simplifiable case for ADRP.
        if (!ADRPMode || *UsesIt != DefsIt->first) {
          UseToReachingDefs[*UsesIt].insert(DefsIt->first);
          // If UsesIt has several reaching definitions, it is not
          // candidate for simplificaton in non-ADRPMode.
          if (!ADRPMode && UseToReachingDefs[*UsesIt].size() > 1)
            NotCandidate.insert(*UsesIt);
        }
      }
    }
  }
  for (SetOfMachineInstr::const_iterator NotCandidateIt = NotCandidate.begin(),
                                         NotCandidateItEnd = NotCandidate.end();
       NotCandidateIt != NotCandidateItEnd; ++NotCandidateIt) {
    DEBUG(dbgs() << "Too many reaching defs: " << **NotCandidateIt << "\n");
    // It would have been better if we could just remove the entry
    // from the map.  Because of that, we have to filter the garbage
    // (second.empty) in the subsequence analysis.
    UseToReachingDefs[*NotCandidateIt].clear();
  }
}

/// Based on the use to defs information (in ADRPMode), compute the
/// opportunities of LOH ADRP-related.
static void computeADRP(const InstrToInstrs &UseToDefs,
                        ARM64FunctionInfo &ARM64FI,
                        const MachineDominatorTree *MDT) {
  DEBUG(dbgs() << "*** Compute LOH for ADRP\n");
  for (InstrToInstrs::const_iterator UseIt = UseToDefs.begin(),
                                     EndUseIt = UseToDefs.end();
       UseIt != EndUseIt; ++UseIt) {
    unsigned Size = UseIt->second.size();
    if (Size == 0)
      continue;
    if (Size == 1) {
      const MachineInstr *L2 = *UseIt->second.begin();
      const MachineInstr *L1 = UseIt->first;
      if (!MDT->dominates(L2, L1)) {
        DEBUG(dbgs() << "Dominance check failed:\n" << *L2 << '\n' << *L1
                     << '\n');
        continue;
      }
      DEBUG(dbgs() << "Record AdrpAdrp:\n" << *L2 << '\n' << *L1 << '\n');
      SmallVector<const MachineInstr *, 2> Args;
      Args.push_back(L2);
      Args.push_back(L1);
      ARM64FI.addLOHDirective(MCLOH_AdrpAdrp, Args);
      ++NumADRPSimpleCandidate;
    }
#ifdef DEBUG
    else if (Size == 2)
      ++NumADRPComplexCandidate2;
    else if (Size == 3)
      ++NumADRPComplexCandidate3;
    else
      ++NumADRPComplexCandidateOther;
#endif
    // if Size < 1, the use should have been removed from the candidates
    assert(Size >= 1 && "No reaching defs for that use!");
  }
}

/// Check whether the given instruction can be the end of a LOH chain
/// involving a load.
static bool isCandidateLoad(const MachineInstr *Instr) {
  switch (Instr->getOpcode()) {
  default:
    return false;
  case ARM64::LDRSBWui:
  case ARM64::LDRSBXui:
  case ARM64::LDRSHWui:
  case ARM64::LDRSHXui:
  case ARM64::LDRSWui:
  case ARM64::LDRBui:
  case ARM64::LDRHui:
  case ARM64::LDRWui:
  case ARM64::LDRXui:
  case ARM64::LDRSui:
  case ARM64::LDRDui:
  case ARM64::LDRQui:
    if (Instr->getOperand(2).getTargetFlags() & ARM64II::MO_GOT)
      return false;
    return true;
  }
  // Unreachable.
  return false;
}

/// Check whether the given instruction can load a litteral.
static bool supportLoadFromLiteral(const MachineInstr *Instr) {
  switch (Instr->getOpcode()) {
  default:
    return false;
  case ARM64::LDRSWui:
  case ARM64::LDRWui:
  case ARM64::LDRXui:
  case ARM64::LDRSui:
  case ARM64::LDRDui:
  case ARM64::LDRQui:
    return true;
  }
  // Unreachable.
  return false;
}

/// Check whether the given instruction is a LOH candidate.
/// \param UseToDefs is used to check that Instr is at the end of LOH supported
/// chain.
/// \pre UseToDefs contains only on def per use, i.e., obvious non candidate are
/// already been filtered out.
static bool isCandidate(const MachineInstr *Instr,
                        const InstrToInstrs &UseToDefs,
                        const MachineDominatorTree *MDT) {
  if (!isCandidateLoad(Instr) && !isCandidateStore(Instr))
    return false;

  const MachineInstr *Def = *UseToDefs.find(Instr)->second.begin();
  if (Def->getOpcode() != ARM64::ADRP) {
    // At this point, Def is ADDXri or LDRXui of the right type of
    // symbol, because we filtered out the uses that were not defined
    // by these kind of instructions (+ ADRP).

    // Check if this forms a simple chain: each intermediate node must
    // dominates the next one.
    if (!MDT->dominates(Def, Instr))
      return false;
    // Move one node up in the simple chain.
    if (UseToDefs.find(Def) == UseToDefs.end()
                               // The map may contain garbage we have to ignore.
        ||
        UseToDefs.find(Def)->second.empty())
      return false;
    Instr = Def;
    Def = *UseToDefs.find(Def)->second.begin();
  }
  // Check if we reached the top of the simple chain:
  // - top is ADRP.
  // - check the simple chain property: each intermediate node must
  // dominates the next one.
  if (Def->getOpcode() == ARM64::ADRP)
    return MDT->dominates(Def, Instr);
  return false;
}

static bool registerADRCandidate(const MachineInstr *Use,
                                 const InstrToInstrs &UseToDefs,
                                 const InstrToInstrs *DefsPerColorToUses,
                                 ARM64FunctionInfo &ARM64FI,
                                 SetOfMachineInstr *InvolvedInLOHs,
                                 const MapRegToId &RegToId) {
  // Look for opportunities to turn ADRP -> ADD or
  // ADRP -> LDR GOTPAGEOFF into ADR.
  // If ADRP has more than one use. Give up.
  if (Use->getOpcode() != ARM64::ADDXri &&
      (Use->getOpcode() != ARM64::LDRXui ||
       !(Use->getOperand(2).getTargetFlags() & ARM64II::MO_GOT)))
    return false;
  InstrToInstrs::const_iterator It = UseToDefs.find(Use);
  // The map may contain garbage that we need to ignore.
  if (It == UseToDefs.end() || It->second.empty())
    return false;
  const MachineInstr *Def = *It->second.begin();
  if (Def->getOpcode() != ARM64::ADRP)
    return false;
  // Check the number of users of ADRP.
  const SetOfMachineInstr *Users =
      getUses(DefsPerColorToUses,
              RegToId.find(Def->getOperand(0).getReg())->second, Def);
  if (Users->size() > 1) {
    ++NumADRComplexCandidate;
    return false;
  }
  ++NumADRSimpleCandidate;
  assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Def)) &&
         "ADRP already involved in LOH.");
  assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Use)) &&
         "ADD already involved in LOH.");
  DEBUG(dbgs() << "Record AdrpAdd\n" << *Def << '\n' << *Use << '\n');

  SmallVector<const MachineInstr *, 2> Args;
  Args.push_back(Def);
  Args.push_back(Use);

  ARM64FI.addLOHDirective(Use->getOpcode() == ARM64::ADDXri ? MCLOH_AdrpAdd
                                                            : MCLOH_AdrpLdrGot,
                          Args);
  return true;
}

/// Based on the use to defs information (in non-ADRPMode), compute the
/// opportunities of LOH non-ADRP-related
static void computeOthers(const InstrToInstrs &UseToDefs,
                          const InstrToInstrs *DefsPerColorToUses,
                          ARM64FunctionInfo &ARM64FI, const MapRegToId &RegToId,
                          const MachineDominatorTree *MDT) {
  SetOfMachineInstr *InvolvedInLOHs = NULL;
#ifdef DEBUG
  SetOfMachineInstr InvolvedInLOHsStorage;
  InvolvedInLOHs = &InvolvedInLOHsStorage;
#endif // DEBUG
  DEBUG(dbgs() << "*** Compute LOH for Others\n");
  // ADRP -> ADD/LDR -> LDR/STR pattern.
  // Fall back to ADRP -> ADD pattern if we fail to catch the bigger pattern.

  // FIXME: When the statistics are not important,
  // This initial filtering loop can be merged into the next loop.
  // Currently, we didn't do it to have the same code for both DEBUG and
  // NDEBUG builds. Indeed, the iterator of the second loop would need
  // to be changed.
  SetOfMachineInstr PotentialCandidates;
  SetOfMachineInstr PotentialADROpportunities;
  for (InstrToInstrs::const_iterator UseIt = UseToDefs.begin(),
                                     EndUseIt = UseToDefs.end();
       UseIt != EndUseIt; ++UseIt) {
    // If no definition is available, this is a non candidate.
    if (UseIt->second.empty())
      continue;
    // Keep only instructions that are load or store and at the end of
    // a ADRP -> ADD/LDR/Nothing chain.
    // We already filtered out the no-chain cases.
    if (!isCandidate(UseIt->first, UseToDefs, MDT)) {
      PotentialADROpportunities.insert(UseIt->first);
      continue;
    }
    PotentialCandidates.insert(UseIt->first);
  }

  // Make the following distinctions for statistics as the linker does
  // know how to decode instructions:
  // - ADD/LDR/Nothing make there different patterns.
  // - LDR/STR make two different patterns.
  // Hence, 6 - 1 base patterns.
  // (because ADRP-> Nothing -> STR is not simplifiable)

  // The linker is only able to have a simple semantic, i.e., if pattern A
  // do B.
  // However, we want to see the opportunity we may miss if we were able to
  // catch more complex cases.

  // PotentialCandidates are result of a chain ADRP -> ADD/LDR ->
  // A potential candidate becomes a candidate, if its current immediate
  // operand is zero and all nodes of the chain have respectively only one user
  SetOfMachineInstr::const_iterator CandidateIt, EndCandidateIt;
#ifdef DEBUG
  SetOfMachineInstr DefsOfPotentialCandidates;
#endif
  for (CandidateIt = PotentialCandidates.begin(),
      EndCandidateIt = PotentialCandidates.end();
       CandidateIt != EndCandidateIt; ++CandidateIt) {
    const MachineInstr *Candidate = *CandidateIt;
    // Get the definition of the candidate i.e., ADD or LDR.
    const MachineInstr *Def = *UseToDefs.find(Candidate)->second.begin();
    // Record the elements of the chain.
    const MachineInstr *L1 = Def;
    const MachineInstr *L2 = NULL;
    unsigned ImmediateDefOpc = Def->getOpcode();
    if (Def->getOpcode() != ARM64::ADRP) {
      // Check the number of users of this node.
      const SetOfMachineInstr *Users =
          getUses(DefsPerColorToUses,
                  RegToId.find(Def->getOperand(0).getReg())->second, Def);
      if (Users->size() > 1) {
#ifdef DEBUG
        // if all the uses of this def are in potential candidate, this is
        // a complex candidate of level 2.
        SetOfMachineInstr::const_iterator UseIt = Users->begin();
        SetOfMachineInstr::const_iterator EndUseIt = Users->end();
        for (; UseIt != EndUseIt; ++UseIt) {
          if (!PotentialCandidates.count(*UseIt)) {
            ++NumTooCplxLvl2;
            break;
          }
        }
        if (UseIt == EndUseIt)
          ++NumCplxLvl2;
#endif // DEBUG
        PotentialADROpportunities.insert(Def);
        continue;
      }
      L2 = Def;
      Def = *UseToDefs.find(Def)->second.begin();
      L1 = Def;
    } // else the element in the middle of the chain is nothing, thus
      // Def already contains the first element of the chain.

    // Check the number of users of the first node in the chain, i.e., ADRP
    const SetOfMachineInstr *Users =
        getUses(DefsPerColorToUses,
                RegToId.find(Def->getOperand(0).getReg())->second, Def);
    if (Users->size() > 1) {
#ifdef DEBUG
      // if all the uses of this def are in the defs of the potential candidate,
      // this is a complex candidate of level 1
      if (DefsOfPotentialCandidates.empty()) {
        // lazy init
        DefsOfPotentialCandidates = PotentialCandidates;
        for (SetOfMachineInstr::const_iterator
                 It = PotentialCandidates.begin(),
                 EndIt = PotentialCandidates.end();
             It != EndIt; ++It)
          if (!UseToDefs.find(Candidate)->second.empty())
            DefsOfPotentialCandidates.insert(
                *UseToDefs.find(Candidate)->second.begin());
      }
      SetOfMachineInstr::const_iterator UseIt = Users->begin();
      SetOfMachineInstr::const_iterator EndUseIt = Users->end();
      for (; UseIt != EndUseIt; ++UseIt) {
        if (!DefsOfPotentialCandidates.count(*UseIt)) {
          ++NumTooCplxLvl1;
          break;
        }
      }
      if (UseIt == EndUseIt)
        ++NumCplxLvl1;
#endif // DEBUG
      continue;
    }

    bool IsL2Add = (ImmediateDefOpc == ARM64::ADDXri);
    // If the chain is three instructions long and ldr is the second element,
    // then this ldr must load form GOT, otherwise this is not a correct chain.
    if (L2 && !IsL2Add && L2->getOperand(2).getTargetFlags() != ARM64II::MO_GOT)
      continue;
    SmallVector<const MachineInstr *, 3> Args;
    MCLOHType Kind;
    if (isCandidateLoad(Candidate)) {
      if (L2 == NULL) {
        // At this point, the candidate LOH indicates that the ldr instruction
        // may use a direct access to the symbol. There is not such encoding
        // for loads of byte and half.
        if (!supportLoadFromLiteral(Candidate))
          continue;

        DEBUG(dbgs() << "Record AdrpLdr:\n" << *L1 << '\n' << *Candidate
                     << '\n');
        Kind = MCLOH_AdrpLdr;
        Args.push_back(L1);
        Args.push_back(Candidate);
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L1)) &&
               "L1 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Candidate)) &&
               "Candidate already involved in LOH.");
        ++NumADRPToLDR;
      } else {
        DEBUG(dbgs() << "Record Adrp" << (IsL2Add ? "Add" : "LdrGot")
                     << "Ldr:\n" << *L1 << '\n' << *L2 << '\n' << *Candidate
                     << '\n');

        Kind = IsL2Add ? MCLOH_AdrpAddLdr : MCLOH_AdrpLdrGotLdr;
        Args.push_back(L1);
        Args.push_back(L2);
        Args.push_back(Candidate);

        PotentialADROpportunities.remove(L2);
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L1)) &&
               "L1 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L2)) &&
               "L2 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Candidate)) &&
               "Candidate already involved in LOH.");
#ifdef DEBUG
        // get the immediate of the load
        if (Candidate->getOperand(2).getImm() == 0)
          if (ImmediateDefOpc == ARM64::ADDXri)
            ++NumADDToLDR;
          else
            ++NumLDRToLDR;
        else if (ImmediateDefOpc == ARM64::ADDXri)
          ++NumADDToLDRWithImm;
        else
          ++NumLDRToLDRWithImm;
#endif // DEBUG
      }
    } else {
      if (ImmediateDefOpc == ARM64::ADRP)
        continue;
      else {

        DEBUG(dbgs() << "Record Adrp" << (IsL2Add ? "Add" : "LdrGot")
                     << "Str:\n" << *L1 << '\n' << *L2 << '\n' << *Candidate
                     << '\n');

        Kind = IsL2Add ? MCLOH_AdrpAddStr : MCLOH_AdrpLdrGotStr;
        Args.push_back(L1);
        Args.push_back(L2);
        Args.push_back(Candidate);

        PotentialADROpportunities.remove(L2);
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L1)) &&
               "L1 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L2)) &&
               "L2 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Candidate)) &&
               "Candidate already involved in LOH.");
#ifdef DEBUG
        // get the immediate of the store
        if (Candidate->getOperand(2).getImm() == 0)
          if (ImmediateDefOpc == ARM64::ADDXri)
            ++NumADDToSTR;
          else
            ++NumLDRToSTR;
        else if (ImmediateDefOpc == ARM64::ADDXri)
          ++NumADDToSTRWithImm;
        else
          ++NumLDRToSTRWithImm;
#endif // DEBUG
      }
    }
    ARM64FI.addLOHDirective(Kind, Args);
  }

  // Now, we grabbed all the big patterns, check ADR opportunities.
  for (SetOfMachineInstr::const_iterator
           CandidateIt = PotentialADROpportunities.begin(),
           EndCandidateIt = PotentialADROpportunities.end();
       CandidateIt != EndCandidateIt; ++CandidateIt)
    registerADRCandidate(*CandidateIt, UseToDefs, DefsPerColorToUses, ARM64FI,
                         InvolvedInLOHs, RegToId);
}

/// Look for every register defined by potential LOHs candidates.
/// Map these registers with dense id in @p RegToId and vice-versa in
/// @p IdToReg. @p IdToReg is populated only in DEBUG mode.
static void collectInvolvedReg(MachineFunction &MF, MapRegToId &RegToId,
                               MapIdToReg &IdToReg,
                               const TargetRegisterInfo *TRI) {
  unsigned CurRegId = 0;
  if (!PreCollectRegister) {
    unsigned NbReg = TRI->getNumRegs();
    for (; CurRegId < NbReg; ++CurRegId) {
      RegToId[CurRegId] = CurRegId;
      DEBUG(IdToReg.push_back(CurRegId));
      DEBUG(assert(IdToReg[CurRegId] == CurRegId && "Reg index mismatches"));
    }
    return;
  }

  DEBUG(dbgs() << "** Collect Involved Register\n");
  for (MachineFunction::const_iterator IMBB = MF.begin(), IMBBEnd = MF.end();
       IMBB != IMBBEnd; ++IMBB)
    for (MachineBasicBlock::const_iterator II = IMBB->begin(),
                                           IEnd = IMBB->end();
         II != IEnd; ++II) {

      if (!canDefBePartOfLOH(II))
        continue;

      // Process defs
      for (MachineInstr::const_mop_iterator IO = II->operands_begin(),
                                            IOEnd = II->operands_end();
           IO != IOEnd; ++IO) {
        if (!IO->isReg() || !IO->isDef())
          continue;
        unsigned CurReg = IO->getReg();
        for (MCRegAliasIterator AI(CurReg, TRI, true); AI.isValid(); ++AI)
          if (RegToId.find(*AI) == RegToId.end()) {
            DEBUG(IdToReg.push_back(*AI);
                  assert(IdToReg[CurRegId] == *AI &&
                         "Reg index mismatches insertion index."));
            RegToId[*AI] = CurRegId++;
            DEBUG(dbgs() << "Register: " << PrintReg(*AI, TRI) << '\n');
          }
      }
    }
}

bool ARM64CollectLOH::runOnMachineFunction(MachineFunction &Fn) {
  const TargetMachine &TM = Fn.getTarget();
  const TargetRegisterInfo *TRI = TM.getRegisterInfo();
  const MachineDominatorTree *MDT = &getAnalysis<MachineDominatorTree>();

  MapRegToId RegToId;
  MapIdToReg IdToReg;
  ARM64FunctionInfo *ARM64FI = Fn.getInfo<ARM64FunctionInfo>();
  assert(ARM64FI && "No MachineFunctionInfo for this function!");

  DEBUG(dbgs() << "Looking for LOH in " << Fn.getName() << '\n');

  collectInvolvedReg(Fn, RegToId, IdToReg, TRI);
  if (RegToId.empty())
    return false;

  MachineInstr *DummyOp = NULL;
  if (BasicBlockScopeOnly) {
    const ARM64InstrInfo *TII =
        static_cast<const ARM64InstrInfo *>(TM.getInstrInfo());
    // For local analysis, create a dummy operation to record uses that are not
    // local.
    DummyOp = Fn.CreateMachineInstr(TII->get(ARM64::COPY), DebugLoc());
  }

  unsigned NbReg = RegToId.size();
  bool Modified = false;

  // Start with ADRP.
  InstrToInstrs *ColorOpToReachedUses = new InstrToInstrs[NbReg];

  // Compute the reaching def in ADRP mode, meaning ADRP definitions
  // are first considered as uses.
  reachingDef(&Fn, ColorOpToReachedUses, RegToId, true, DummyOp);
  DEBUG(dbgs() << "ADRP reaching defs\n");
  DEBUG(printReachingDef(ColorOpToReachedUses, NbReg, TRI, IdToReg));

  // Translate the definition to uses map into a use to definitions map to ease
  // statistic computation.
  InstrToInstrs ADRPToReachingDefs;
  reachedUsesToDefs(ADRPToReachingDefs, ColorOpToReachedUses, RegToId, true);

  // Compute LOH for ADRP.
  computeADRP(ADRPToReachingDefs, *ARM64FI, MDT);
  delete[] ColorOpToReachedUses;

  // Continue with general ADRP -> ADD/LDR -> LDR/STR pattern.
  ColorOpToReachedUses = new InstrToInstrs[NbReg];

  // first perform a regular reaching def analysis.
  reachingDef(&Fn, ColorOpToReachedUses, RegToId, false, DummyOp);
  DEBUG(dbgs() << "All reaching defs\n");
  DEBUG(printReachingDef(ColorOpToReachedUses, NbReg, TRI, IdToReg));

  // Turn that into a use to defs to ease statistic computation.
  InstrToInstrs UsesToReachingDefs;
  reachedUsesToDefs(UsesToReachingDefs, ColorOpToReachedUses, RegToId, false);

  // Compute other than AdrpAdrp LOH.
  computeOthers(UsesToReachingDefs, ColorOpToReachedUses, *ARM64FI, RegToId,
                MDT);
  delete[] ColorOpToReachedUses;

  if (BasicBlockScopeOnly)
    Fn.DeleteMachineInstr(DummyOp);

  return Modified;
}

/// createARM64CollectLOHPass - returns an instance of the Statistic for
/// linker optimization pass.
FunctionPass *llvm::createARM64CollectLOHPass() {
  return new ARM64CollectLOH();
}
