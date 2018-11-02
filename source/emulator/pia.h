/*
 * pia.h
 *
 *  Created on: Nov 1, 2018
 *      Author: ned
 */

#ifndef PIA_H_
#define PIA_H_


// PIAs are always external.
class PIA {
public:
	PIA(uint16_t baseAddress)
		: baseAddress_(baseAddress) { }

	enum RegisterOffset : uint16_t {
		REG_PRA_DDRA = 0,
		REG_CRA = 1,
		REG_PRB_DDRB = 2,
		REG_CRB = 3
	};
	enum ControlRegisterBit : uint8_t {
		CR_IRQ1 = 0x80,	// read-only IRQ1 flag
		CR_IRQ2 = 0x40,	// read-only IRQ2 flag
		// b5..b3: CA2(CB2) control
		CR_CAB2_OUTPUT = 0x20,	// CR b5 = 1: C[AB]2 is an output
		CR_CA2_READ_STROBE_CA1_RESTORE = 4 << 3,
		CR_CA2_READ_STROBE_E_RESTORE = 5 << 3,
		CR_CB2_WRITE_STROBE_CB1_RESTORE = 4 << 3,
		CR_CB2_WRITE_STROBE_E_RESTORE = 5 << 3,
		CR_RESET_CAB2 = 6 << 3,
		CR_SET_CAB2 = 7 << 3,
		CR_CAB2_IRQ_ENABLE = 1 << 3,
		CR_CAB2_IRQ_HIGH_EDGE = 1 << 3,
		CR_PR_ACCESS = 0x04,	// 0=DDR, 1=Output reg
		CR_CAB1_IRQ_HIGH_EDGE = 0x02,
		CR_CAB1_IRQ_ENABLE = 0x01
	};

	// DDR bits: 1: output, 0: input
	void setDataDirectionA(uint8_t outputBits, uint8_t mask) {
		uint8_t crA = registerRead(REG_CRA);
		registerWrite(REG_CRA, crA & ~CR_PR_ACCESS);

		uint8_t ddrA = registerRead(REG_PRA_DDRA) & ~mask;	// keep non-mask bits
		registerWrite(REG_PRA_DDRA, ddrA | outputBits);
	}

	void setDataDirectionB(uint8_t outputBits, uint8_t mask) {
		uint8_t crB = registerRead(REG_CRB);
		registerWrite(REG_CRB, crB & ~CR_PR_ACCESS);

		uint8_t ddrB = registerRead(REG_PRB_DDRB) & ~mask;	// keep non-mask bits
		registerWrite(REG_PRB_DDRB, ddrB | outputBits);
	}

	void outputA(uint8_t outputBits, uint8_t mask) {
		uint8_t cr = registerRead(REG_CRA);
		registerWrite(REG_CRA, cr | CR_PR_ACCESS);	// select PRA
		uint8_t pr = registerRead(REG_PRA_DDRA);
		registerWrite(REG_PRA_DDRA, (pr & ~mask) | outputBits);
	}

	void outputB(uint8_t outputBits, uint8_t mask) {
		uint8_t cr = registerRead(REG_CRA);
		registerWrite(REG_CRB, cr | CR_PR_ACCESS);	// select PRB
		uint8_t pr = registerRead(REG_PRB_DDRB);
		registerWrite(REG_PRB_DDRB, (pr & ~mask) | outputBits);
	}

protected:
	void registerWrite(RegisterOffset offset, uint8_t value) {
		cpu_writemem_external(baseAddress_ + static_cast<uint16_t>(offset),	value);
	}
	uint8_t registerRead(RegisterOffset offset) {
		return cpu_readmem_external(baseAddress_ + static_cast<uint16_t>(offset));
	}

	const uint16_t baseAddress_;
};	// class PIA



#endif /* PIA_H_ */
