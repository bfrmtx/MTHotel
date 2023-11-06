#ifndef ATMHEADER
#define ATMHEADER

#include <array>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

namespace fs = std::filesystem;

//  ATM - mask

/*!
   \brief The atmheader struct; true if grey shaded ; true == exclude from processing

*/

struct atmheader {
  std::int16_t header_length;
  std::int16_t header_version;
  uint32_t samples;
};

class atmfile {

public:
  /*!
   * \brief atmfile
   */
  atmfile() {
    this->header.header_length = 8;
    this->header.header_version = 10;
    this->header.samples = 0;
  }

  /*!
   * \brief atsheader
   * \param filename file to open for reading the header
   */
  atmfile(const fs::path &filename) {
    this->read(filename, true);
  }
  atmfile(const atmfile &rhs) {
    this->header = rhs.header;
    this->filename = rhs.filename;
  }

  atmfile(const std::shared_ptr<atmfile> &rhs) {
    this->header = rhs->header;
    this->filename = rhs->filename;
  }

  ~atmfile() {
    if (file.is_open())
      file.close();
  }

  bool read(const fs::path &filename = "", const bool close_after_read = false) {
    if (filename.empty() && this->filename.empty())
      return false;
    else if (!filename.empty())
      this->filename = filename;
    if (this->filename.empty())
      return false;
    this->file.open(this->filename, std::ios::in | std::ios::binary);
    if (this->file.is_open()) {
      this->file.read((char *)&this->header, sizeof(this->header));
    }
    if (close_after_read)
      this->file.close();
    return true;
  }

  size_t close(const bool write_data = true) {
    if (file.is_open())
      file.close();
    if (write_data) {
      return this->flush_data();
    }
    return this->sel.size();
  }

  size_t flush_data() {
    if (file.is_open())
      file.close();
    this->header.samples = uint32_t(this->sel.size());

    if (!this->write())
      return 0;
    return this->write_vector();
  }

  bool write(const bool close_after_write = false) {
    if (!sizeof(this->filename.filename()))
      return false;
    if (this->file.is_open())
      this->file.close();

    this->file.open(filename, std::ios::out | std::ios::binary);
    if (this->file.is_open()) {
      this->file.write((char *)&this->header, sizeof(this->header));
    } else {
      this->file.close();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: can not WRITE HEADER! " << this->filename;
      throw std::runtime_error(err_str.str());
    }
    if (close_after_write)
      this->file.close();
    return true;
  }

  bool re_write() {
    if (!sizeof(this->filename.filename()))
      return false;
    if (this->file.is_open())
      this->file.close();

    this->file.open(filename, std::ios::out | std::ios::in | std::ios::binary);
    if (this->file.is_open()) {
      this->file.write((char *)&this->header, sizeof(this->header));
    } else {
      this->file.close();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: can not RE-WRITE HEADER! " << this->filename;
      throw std::runtime_error(err_str.str());
    }
    this->file.close();
    return true;
  }

  size_t add_unselected(const size_t &n) {
    std::vector<bool> tsel(n, false);
    this->sel.insert(this->sel.end(), tsel.begin(), tsel.end());
    return this->sel.size();
  }

  size_t add_selected(const size_t &n) {
    std::vector<bool> tsel(n, true);
    this->sel.insert(this->sel.end(), tsel.begin(), tsel.end());
    return this->sel.size();
  }

  void get_atsfile_name(const fs::path &fname) {
    this->filename.clear();
    this->filename = fname.parent_path();
    this->filename /= fname.stem();
    this->filename += ".atm";
  }

  /*!
   * \brief path
   * \return path (and therewith filename)
   */
  fs::path path() const {
    return this->filename;
  }

  void change_dir(const fs::path &new_dir) {
    fs::path fname = this->filename.filename();
    this->filename.clear();
    this->filename = new_dir;
    this->filename /= fname;
  }

  atmheader header; //!< the binary header of 1024 bytes
  std::vector<bool> sel;

private:
  std::fstream file;
  fs::path filename;
  std::bitset<8> bs;
  const unsigned short int charbits = 8 * sizeof(char);

  size_t write_vector() {
    if (!file.is_open())
      return 0;
    size_t i, j;
    char ch;
    for (i = 0; i < this->sel.size() / charbits; i++) {
      for (j = 0; j < charbits; j++) {
        this->bs[j] = this->sel[charbits * i + j];
      }
      ch = char(bs.to_ulong());
      this->file.write((char *)&ch, sizeof(char));
    }

    if (this->sel.size() % charbits) {
      this->bs.reset();
      for (j = 0; j < this->sel.size() % charbits; j++) {
        this->bs[j] = this->sel[charbits * i + j];
      }
      // https://stackoverflow.com/questions/16944617/qbitarray-size-stays-zero-after-trying-to-fill-it-with-stream-operator

      // Qt does a rounding and read/write more bits
      // for the old standard it will be compatible
      // QBitArray consists of a 32bit smaples and following bits
      // the bits are rounded by 8+1 or similar
      // qds.setByteOrder(QDataStream::LittleEndian);
      // qds << qint16 (this->atm.siHeaderLength);
      // qds << qint16 (this->atm.siHeaderVers);
      // // qds << quint32(this->b.size()); // NO!
      // // the selector has a quint32 header
      // qds << b;
      ch = char(bs.to_ulong());
      this->file.write((char *)&ch, sizeof(char));
      this->bs.reset();
      for (size_t k = 0; k < 4; ++k) {
        ch = char(bs.to_ulong());
        this->file.write((char *)&ch, sizeof(char));
      }
    }

    if (file.is_open())
      file.close();
    return this->sel.size();
  }

  /*
  size_t atmfile::read(vector<bool>& vb) {

  size_t i, j;
  char ch;
  if (!is_open) {
      cerr << "atmfile::read -> atm file "<< fname << "  is not open yet. Please use the atm.open(filename) function" << endl;
      return 0;
  }

  if (is_new) {
      cerr << "atmfile::read -> atm file "<< fname << "  is NEW. Please use the write function" << endl;
      return 0;
  }

  // prevent read error by a read vector of wrong dimension
  if (vb.size() != ath.iSamples) {
      cerr << "atmfile::read -> can not read more samples than inside the file "<< fname << " " << endl;
      cerr << "file: " << ath.iSamples << " vector size: " << vb.size() << endl;
      return 0;
  }

  // make sure we start at the right position
  myfile.seekg(sizeof(ath));
  myfile.seekp(sizeof(ath));

  for ( i = 0; i < vb.size() / charbits; i++) {
      myfile.read((char*) &ch, sizeof(char));
      bs = ch;
      for (j = 0; j < charbits; j++)
          vb[charbits * i + j] = bs[j];
  }

  if (vb.size() % charbits) {
      bs.reset();
      myfile.read((char*) &ch, sizeof(char));
      bs = ch;
      for (j = 0; j < vb.size() % charbits; j++)
          vb[charbits * i + j] = bs[j];
  }
  myfile.clear();

  return vb.size();
}

  */
};

#endif // ATMHEADER
