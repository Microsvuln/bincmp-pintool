/// 1. Monitor cmp instruction execution
/// 2. Generating TOKENS based on cmp instructions
////
//// CMP instruction execution trace - Intel PIN
//// @Microsvuln
//// 

#include "pin.H"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

using std::hex;
using std::cerr;
using std::string;
using std::ios;
using std::endl;

std::ofstream TraceFile;


unsigned int i=33333; /// RANDOM high number
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "bincmp.out", "specify trace file name");

VOID writeToken(ADDRINT val)
{
        int fd;
        char line[30];
        char filename[300] = "dict/";      
        char tmp[8];
        char str[16];
        snprintf(str, sizeof(str), "%d", i);
        strcat(filename,str);
        sprintf(tmp, "\\x%02x", (unsigned int)val);    
        strcat(filename, str);
        fd=open(filename,O_CREAT | O_WRONLY,S_IRUSR);
        strcat(line, tmp);
        write(fd,line,strlen(line));
        i--;
        close(fd);
}
VOID print_cmp_mem(VOID *ip, UINT64 * addr, ADDRINT value, ADDRINT * addr2) {

        ADDRINT value2;
	PIN_SafeCopy(&value2, addr2, sizeof(ADDRINT));
        writeToken(value2);

}

VOID print_cmp_reg(VOID *ip, ADDRINT lvalue, ADDRINT rvalue) {

        TraceFile << "cmp" << "::: " << (UINT64)ip << ":" << lvalue << ":" << rvalue << "\n";
        writeToken(lvalue);
        writeToken(rvalue);

}

VOID Instruction(INS ins, VOID *v)
{
    if (INS_Opcode(ins) == XED_ICLASS_CMP) {
        if (INS_MemoryOperandCount(ins) == 1) {
            if (INS_OperandIsImmediate(ins, 1)) {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)print_cmp_mem, IARG_INST_PTR, IARG_MEMORYOP_EA, 0, IARG_ADDRINT, INS_OperandImmediate(ins, 1), IARG_END);
            } else if (INS_OperandIsReg(ins, 0)) {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)print_cmp_mem, IARG_INST_PTR, IARG_MEMORYOP_EA, 0, IARG_REG_VALUE, INS_OperandReg(ins, 0), IARG_END);
            } else {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)print_cmp_mem, IARG_INST_PTR, IARG_MEMORYOP_EA, 0, IARG_REG_VALUE, INS_OperandReg(ins, 1), IARG_END);
            }
        } else {
            if (INS_OperandIsImmediate(ins, 1)) {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)print_cmp_reg, IARG_INST_PTR, IARG_REG_VALUE, INS_OperandReg(ins, 0), IARG_ADDRINT, INS_OperandImmediate(ins, 1), IARG_END);
            } else {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)print_cmp_reg, IARG_INST_PTR, IARG_REG_VALUE, INS_OperandReg(ins, 0), IARG_REG_VALUE, INS_OperandReg(ins, 1), IARG_END);
            }
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    TraceFile.close();
}

int main(int argc, char *argv[])
{
		const int dir_err = mkdir("dict", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err)
        {
            printf("Error creating directory!n");
            exit(1);
        }
		PIN_Init(argc, argv);
		TraceFile.open(KnobOutputFile.Value().c_str());
		TraceFile << hex;
		TraceFile.setf(ios::showbase);   
		INS_AddInstrumentFunction(Instruction, 0);
		PIN_StartProgram();
		return 0;
}
