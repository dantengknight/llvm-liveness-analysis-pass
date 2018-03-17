#include "Liveness.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/iterator_range.h"

#include <map>
#include <set>
#include <list>
#include <utility>
#include <algorithm>
#include <iterator>


using namespace llvm;
using namespace std;



namespace {
    // Liveness - The first implementation, without getAnalysisUsage.
    struct Liveness : public FunctionPass {
        static char ID; // Pass identification, replacement for typeid
        Liveness() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            // build a map for UE/VK, key: BB pointer, value: live var/killed
            map<BasicBlock*, set<llvm::StringRef>> UE;
            map<BasicBlock*, set<llvm::StringRef>> VK;
            map<BasicBlock*, set<llvm::StringRef>>::iterator it;
            map<BasicBlock*, set<llvm::StringRef>>::iterator kit;
            for (BasicBlock &BB : F) {
                // init UEVar/VarKill
                set<StringRef> UEVar;
                UE.insert(std::pair<BasicBlock*, std::set<llvm::StringRef>>(&BB,UEVar));
                set<StringRef> VarKill;
                VK.insert(std::pair<BasicBlock*, std::set<llvm::StringRef>>(&BB,VarKill));
                for (Instruction &I : BB) {
                    //errs() << "Instruction: " << I << '\n';
                    //errs() << "# of operands: " << I.getNumOperands() << '\n';
                    //errs() << I.getOpcode() << '\n'; 
                    
                    for (unsigned i = 0, e = I.getNumOperands(); i != e; i++) {

                        StringRef varName = I.getOperand(i)->getName();

                        // put used var into UE, except for 2nd operand for
                        // store 
                        if (I.getOpcode() == 1 || I.getOpcode() == 2 || 
                                I.getOpcode() == 51 || I.getOpcode() == 54)
                            continue;
                        // check Varkill
                        kit = VK.find(&BB);
                        if (kit->second.find(varName) != kit->second.end())
                            continue;

                        // put var into UEVar
                        it = UE.find(&BB);
                        UEVar = it->second;
                        // insert current name, except for 2nd operand for
                        // store
                        if (!(I.getOpcode() == 31 && i == 1)) {
                            UEVar.insert(varName);
                        }
                        UE.erase(it);
                        UE.insert(std::pair<BasicBlock*, std::set<llvm::StringRef>>(&BB,UEVar));

                        // add 2nd operand for store to Varkill
                        if (I.getOpcode() == 31 && i == 1) {
                            // put var into Varkill
                            kit = VK.find(&BB);
                            VarKill = kit->second;
                            // insert current name (LHS)
                            VarKill.insert(varName);
                            VK.erase(kit);
                            VK.insert(std::pair<BasicBlock*, std::set<llvm::StringRef>>(&BB,VarKill));
                        }
                    }
                    // insert varkill, except for store op
                    if (!(I.getOpcode() == 1 || I.getOpcode() == 2 || I.getOpcode() == 31
                           || I.getOpcode() == 51 || I.getOpcode() == 54)) {
                        // put var into Varkill
                        kit = VK.find(&BB);
                        VarKill = kit->second;
                        // insert current name (LHS)
                        VarKill.insert(I.getName());
                        VK.erase(kit);
                        VK.insert(std::pair<BasicBlock*, std::set<llvm::StringRef>>(&BB,VarKill));
                    }
                }

            }
            // print out the BB map for varkill
            errs() << "Varkill Set: " << '\n';
            for (map<BasicBlock*, set<llvm::StringRef>>::iterator dit = VK.begin(); dit != VK.end(); dit++) {
                // get key
                BasicBlock* tmpkey = dit->first;
                // get varkill value
                set<StringRef> VarKill = dit->second;
                errs() << "BasicBlock: " << tmpkey->getName() << " -->";
                errs() << " VarKill: {"; 
                for (set<StringRef>::iterator sit = VarKill.begin(); sit != VarKill.end(); sit++) {
                    errs() << *sit << " ";
                }
                errs() << "}" << '\n';
            }
            // print out the BB map for UE
            errs() << "Upward Exposed Set: " << '\n';
            for (map<BasicBlock*, set<llvm::StringRef>>::iterator dit = UE.begin(); dit != UE.end(); dit++) {
                // get key
                BasicBlock* tmpkey = dit->first;
                // get varkill value
                set<StringRef> UEVar = dit->second;
                errs() << "BasicBlock: " << tmpkey->getName() << " -->";
                errs() << " UEVar: {"; 
                for (set<StringRef>::iterator sit = UEVar.begin(); sit != UEVar.end(); sit++) {
                    errs() << *sit << " ";
                }
                errs() << "}" << '\n';
            }


            // compute live ness 
            map<BasicBlock*, set<llvm::StringRef>> LO; // init
            map<BasicBlock*, set<llvm::StringRef>>::iterator lit;
            std::list<BasicBlock*> workList;
            for (BasicBlock &BB : F) {
                set<StringRef> LIVEOUT;
                LO.insert(std::pair<BasicBlock*, std::set<llvm::StringRef>>(&BB,LIVEOUT));
                workList.push_back(&BB);
            }
            while (!workList.empty()) {
                BasicBlock* tmp = workList.front();// BB pointer
                workList.pop_front();
                map<BasicBlock*, set<llvm::StringRef>>::iterator it; 
                it = LO.find(tmp);
                set<llvm::StringRef> originLIVEOUT = it->second;// original LIVEOUT
                // compute liveness
                const TerminatorInst *TInst = tmp->getTerminator();
                set<llvm::StringRef> resultSet;// result LIVEOUT
                for (int i = 0, NSucc = TInst->getNumSuccessors(); i < NSucc; i++) {
                    BasicBlock* succ = TInst->getSuccessor(i);// get BB pointer
                    // get successor's LIVEOUT/VARKILL/UEVAR
                    set<llvm::StringRef> LIVEOUT = LO.find(succ)->second;
                    set<llvm::StringRef> VarKill = VK.find(succ)->second;
                    set<llvm::StringRef> UEVar = UE.find(succ)->second;
                    set<llvm::StringRef> subtrSet (LIVEOUT);
                    for (set<llvm::StringRef>::iterator setIt = VarKill.begin(); setIt != VarKill.end(); setIt++) {
                        subtrSet.erase(*setIt);
                    }
                    std::set_union(UEVar.begin(), UEVar.end(), subtrSet.begin(),
                            subtrSet.end(), std::inserter(resultSet, resultSet.begin()));
                }
                LO.erase(it);// update the LIVEOUT for this BB
                LO.insert(std::pair<BasicBlock*, std::set<llvm::StringRef>>(tmp,resultSet));
                if (resultSet != originLIVEOUT) {// if changed, add predecessor to worklist
                    for (auto predIt = pred_begin(tmp), predEnd = pred_end(tmp); 
                            predIt != predEnd; predIt++) {
                        BasicBlock* pred = *predIt;
                        workList.push_back(pred);
                    }
                }
            }
            // print out the BB map for Liveness
            errs() << "Liveness Set: " << '\n';
            for (map<BasicBlock*, set<llvm::StringRef>>::iterator lit = LO.begin(); lit != LO.end(); lit++) {
                BasicBlock* tmpkey = lit->first;
                set<StringRef> LIVEOUT = lit->second;
                errs() << "BasicBlock: " << tmpkey->getName() << " -->";
                errs() << " LIVEOUT: {"; 
                for (set<StringRef>::iterator sit = LIVEOUT.begin(); sit != LIVEOUT.end(); sit++) {
                    errs() << *sit << " ";
                }
                errs() << "}" << '\n';
            }
            return false;
        }
    };
}

char Liveness::ID = 0;
static RegisterPass<Liveness> X("liveness", "My Liveness Set Pass");

