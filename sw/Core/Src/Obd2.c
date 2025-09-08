#include <Obd2.h>
#include <assert.h>
#include <IsoTp.h>

struct Obd2Data makeObd2Data(enum Obd2Mode mode, enum Obd2Pid pid)
{
	struct Obd2Data data = {
		.mode = mode,
		.pid = pid,
		.n_abcd = 0,
		.abcd = {0xAA, 0xAA, 0xAA, 0xAA}
	};

	return data;
}

static uint8_t packCodeAndSize8b(uint8_t code, uint8_t size)
{
	return (code << 7) | (size & 0x07);
}

void packObd2Data(uint8_t* packed, const struct Obd2Data* data, uint8_t packedLength)
{
	assert(packedLength >= ISO_TP_SF_SIZE);

	uint8_t code = 0;
	uint8_t size = 2 + data->n_abcd;

	packed[0] = packCodeAndSize8b(code, size);
	packed[1] = (uint8_t) data->mode;
	packed[2] = (uint8_t) data->pid;
	for (uint8_t i = 3; i <= 6; ++i)
	{
		packed[i] = data->abcd[i - 3];
	}
}
