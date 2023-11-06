#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, size_t> seat_reservation(const std::vector<std::string> &names, size_t seats) {

  // seats we use it to force an error ans see how the function and main behaves
  std::unordered_map<std::string, size_t> reservation;

  try {
    if (names.size() > seats) {
      // std::string err_str = __func__;
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::can not make reservation: seats: " << seats << ", persons: " << names.size();
      throw std::runtime_error(err_str.str());
    }
    // so seat ok, continue
    size_t i = 0;
    for (const auto &name : names) {
      reservation.emplace(name, i++);
    }

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    std::cout << "function call reservation WITH catched exception" << std::endl;
    std::cerr << "making a fake reservation" << std::endl;

    // so this code will be executed after throw
    // reserve what I have - but I have to ask for names later
    for (size_t i = 0; i < seats; ++i) {
      reservation.emplace("reserved_seat_" + std::to_string(i), i);
    }
  }
  // that would catch others
  catch (...) {
  }

  // that is finally returned
  return reservation;
}

std::unordered_map<std::string, size_t> seat_reservation_wo(const std::vector<std::string> &names, size_t seats) {

  // seats we use it to force an error ans see how the function and main behaves
  std::unordered_map<std::string, size_t> reservation;

  if (names.size() > seats) {
    // std::string err_str = __func__;
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::can not make reservation: seats: " << seats << ", persons: " << names.size();
    std::cout << "function call reservation WITHOUT catched exception" << std::endl;
    throw std::runtime_error(err_str.str());
  }
  // so seat ok, continue
  size_t i = 0;
  for (const auto &name : names) {
    reservation.emplace(name, i++);
  }

  // that is finally returned
  return reservation;
}

int main() {

  std::vector<std::string> persons;
  std::vector<size_t> seats{3, 2};

  persons.emplace_back("Billy");
  persons.emplace_back("Ted");
  persons.emplace_back("Scooter");

  std::unordered_map<std::string, size_t> result;

  for (size_t count = 0; count < 2; ++count) {
    for (const auto &seat : seats) {

      try {
        if (!count) {
          std::cout << "starting reservation MAIN, WITH exeception catched INSIDE function: " << std::endl;
          result = seat_reservation(persons, seat);
        } else {
          std::cout << "starting reservation MAIN, WITHOUT exeception catched INSIDE function: " << std::endl;
          result = seat_reservation_wo(persons, seat);
        }

        if (!result.size()) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << ":: can not make reservation - msg from MAIN";
          throw std::runtime_error(err_str.str());
        } else {
          if (result.size() == persons.size())
            std::cout << "MAIN reservation successful" << std::endl;
          else
            std::cout << "MAIN reservation PROBLEM, solved" << std::endl;
          for (const auto &p : result) {
            std::cout << p.first << " " << p.second << std::endl;
          }
        }

      } catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
        std::cerr << "MAIN could not execute reservation in MAIN" << std::endl;
      }
      std::cout << std::endl;

      result.clear();
    }
  }
  std::cout << std::endl;
  std::cout << "*************************************************" << std::endl;

  std::cout << std::endl;
  std::cout << "************************no catch in main -> program dies *************************" << std::endl;
  std::cout << "throwing an instance of 'std::__cxx11::basic_string" << std::endl;
  std::cout << "will be the error message, because we throw a string" << std::endl
            << std::endl;

  for (const auto &seat : seats) {

    std::cout << "starting reservation MAIN, WITHOUT exeception catched INSIDE function: " << std::endl;
    result = seat_reservation_wo(persons, seat);
    if (!result.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: can not make reservation - msg from MAIN";
      throw std::runtime_error(err_str.str());
    } else {
      if (result.size() == persons.size())
        std::cout << "MAIN reservation successful" << std::endl;
      else
        std::cout << "MAIN reservation PROBLEM, solved" << std::endl;
      for (const auto &p : result) {
        std::cout << p.first << " " << p.second << std::endl;
      }
    }

    std::cout << std::endl;

    result.clear();
  }

  std::cout << "main finished " << std::endl;
  return 0;
}
