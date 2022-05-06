#ifndef MK_CALIBRATION_H
#define MK_CALIBRATION_H

#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <utility>
#include <exception>

#include "synthetic.h"

namespace fs = std::filesystem;


#include "../../include/mt_base.h"
#include "../../include/strings_etc.h"
#include "../../include/about_system.h"
#include "../../sqlite_handler/sqlite_handler.h"
#include "../read_cal/read_cal.h"

/*!
 * \brief The mk_calibration class creates calibration data
 */
class mk_calibration
{
public:
    mk_calibration();
};

#endif // MK_CALIBRATION_H
