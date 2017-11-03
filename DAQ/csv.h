//
// Created by owner on 10/21/2017.
//

#ifndef SUSPENSIONDAQ_CSV_H
#define SUSPENSIONDAQ_CSV_H
namespace csv{
	struct xyz {
		float x;
		float y;
		float z;
	};
	double getTime();
	void outputLine(xyz accel1, xyz accel2, xyz accel3);
}
#endif //SUSPENSIONDAQ_CSV_H
