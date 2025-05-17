#include <SDFS.h>
#include <WiFi.h>
#include "DeviceMulti.h"

/*
 * This emulates a fictional multi-I/O card, not anything RM actually made,
 * and is designed partly to make the DOS drivers easier to prototype.
 *
 * Registers for disk part:
 *  00 - unit/secsz/status
 *  02 - sector count
 *  04 - start LBA
 *  06 - command/error/status
 *  08 - data
 *
 * Registers for clock part:
 *  10 - reading provides data formatted as 6-byte packets for CLOCK$ driver
 *       writing anything updates the clock / resets pointer for next packet
 *
 * Registers for character part:
 *  20 - data register of 1st emulated 8251
 *  22 - status register of 1st emulated 8251 (read only)
 *  24 - data register of 2nd emulated 8251
 *  26 - status register of 2nd emulated 8251 (read only)
 *  28 - data register of 3rd emulated 8251
 *  2A - status register of 3rd emulated 8251 (read only)
 *  2C - data register of 4th emulated 8251
 *  2E - status register of 4th emulated 8251 (read only)
 *
 * Registers for configuration part:
 *  7C - magic number C1 (read only)
 *  7E - magic number 86 (read only)
 *
 * Bits for unit/secsz/status register: (write is unit/secsz)
 *  7 - drq
 *  6 - error
 *  5 - in
 *  4 - busy
 *  3 - sector size MSB (128, 256, 512, 1024)
 *  2 - sector size LSB
 *  1 - unit MSB (giving us max 4 units)
 *  0 - unit LSB
 *
 * Bits for command/error/status register: (write is command)
 *  7 - drq
 *  6 - error
 *  5 - in
 *  4 - busy
 *  3 - error code MSB (see DOS driver error codes)
 *  2 - error code
 *  1 - error code
 *  0 - error code LSB
 *
 * Bits for emulated 8251 status register:
 *  7 - DSR, 1 if connected
 *  6 - SYNDET, always 0 here
 *  5 - framing error, always 0 here
 *  4 - overrun error
 *  3 - parity error, always 0 here
 *  2 - TxE, 1 if ready to write
 *  1 - RxRDY, 1 if received character available
 *  0 - TxRDY, 1 if ready to write
 */

void DeviceMulti::setup(void) {
  char path[] = "/multi0.img";
  int i;
  _status = 0;
  _secsz = 0;
  _unit = 0;
  _error = 0;
  _command = 0;
  _ptr = 0;
  _lba = 0;
  _secct = 0;
  _update_dostime();

  _server = WiFiServer(18600);
  _server.begin();
  _server.setNoDelay(true);

  // Attempt to open a disk image file for each emulated unit
  for (i = 0; i < 4; i++) {
    Serial.print(path);
    if (SDFS.exists(path)) {
      Serial.print(" open ");
      _diskfile[i] = SDFS.open(path, "r+");
    } else {
      Serial.print(" create ");
      _diskfile[i] = SDFS.open(path, "w+");
    }
    if (_diskfile[i]) {
      Serial.println("succeeded");
    } else {
      Serial.println("failed");
    }
    path[6]++; // The digit in the filename
  }

  // Attempt to open an output log file for each emulated unit
  path[6] = '0';
  path[8] = 'l';
  path[9] = 'o';
  for (i = 0; i < 4; i++) {
    Serial.print(path);
    _charfile[i] = SDFS.open(path, "a");
    if (_charfile[i]) {
      Serial.println(" open succeeded");
    } else {
      Serial.println(" open failed");
    }
    path[6]++;
  }
}

void DeviceMulti::loop(void) {
  int i;
  for (i = 0; i < 4; i++) {
    if (!_client[i]) {
      _client[i] = _server.accept();
    } else if (!_client[i].connected()) {
      _client[i].stop();
      _client[i] = _server.accept();
    } else {
      _client[i].flush();
    }
  }

  if (!(_status & 0x10)) { return; }
  switch (_command) {

    case 0x00: // NOP
      switch (_status) {
        case 0x10:
          _noptimeout = make_timeout_time_ms(_lba);
          _status = 0x11;
          break;
        case 0x11:
          if (absolute_time_diff_us(_noptimeout, get_absolute_time()) > 0) {
            _status = 0;
          }
          break;
        default:
          _status = 0;
      }
      break;

    case 0x01: // Read
      switch (_status) {
        case 0x10:
          if (!_diskfile[_unit]) {
            _error = 0x2;
            _status = 0x40;
            break;
          }
          if (_secct * _secsz > sizeof _secbuf) {
            _error = 0xC;
            _status = 0x40;
            break;
          }
          if (!_diskfile[_unit].seek(_lba * _secsz, SeekSet)) {
            _error = 0x6;
            _status = 0x40;
            break;
          }
          _status = 0x11;
          break;

        case 0x11:
          if (
            _diskfile[_unit].readBytes((char *) _secbuf, _secct * _secsz) <
            _secct * _secsz
          ) {
            _error = 0xB;
            _status = 0x40;
            break;
          }
          _status = 0xB2;
          break;

        default:
          _status = 0;
      }
      break;

    case 0x02: // Write
      switch (_status) {
        case 0x10:
          if (!_diskfile[_unit]) {
            _error = 0x2;
            _status = 0x40;
            break;
          }
          if (_secct * _secsz > sizeof _secbuf) {
            _error = 0xC;
            _status = 0x40;
            break;
          }
          if (!_diskfile[_unit].seek(_lba * _secsz, SeekSet)) {
            _error = 0x6;
            _status = 0x40;
            break;
          }
          _status = 0x91;
          break;

        case 0x11:
          if (
            _diskfile[_unit].write(_secbuf, _secct * _secsz) <
            _secct * _secsz
          ) {
            _error = 0xA;
            _status = 0x40;
            break;
          }
          _status = 0x12;
          break;

        case 0x12:
          _diskfile[_unit].flush();
          _status = 0;
          break;

        default:
          _status = 0;
      }
      break;

    default:
      _error = 0xC;
      _status = 0x40;
  }
}

uint8_t DeviceMulti::read(uint8_t address) {
  uint8_t data = 0x00;
  switch (address) {

    // 00-0F: Disk
    case 0x00: // unit/secsz/status
      data = (_status & 0xF0) | (_unit & 0x03);
      switch (_secsz) {
        // 128 bytes means we leave those two bits at zero
        case  256: data |= 0x04; break;
        case  512: data |= 0x08; break;
        case 1024: data |= 0x0C; break;
      }
      break;

    // Only taking the low byte (the last written) here
    case 0x02: data = _secct; break; // Sector count
    case 0x04: data = _lba;   break; // Starting LBA

    case 0x06: // command/error/status
      data = (_status & 0xF0) | (_error & 0x0F);
      break;

    case 0x08: // Data
      // Read transfer in progress?
      if (_status & 0xA0 != 0xA0) { break; }
      data = _secbuf[_ptr++];
      // If finished, clear DRQ
      if (_ptr >= _secct * _secsz) { _status &= ~0x80; }
      break;

    // 10-1F: CLOCK$
    case 0x10:
      data = _dostime[_dostimeptr++];
      _dostimeptr %= 6;
      break;

    // 20-2F: Character
    case 0x20: // Data
    case 0x24:
    case 0x28:
    case 0x2C:
      if (_client[(address >> 2) & 3]) {
        data = _client[(address >> 2) & 3].read();
      }
      break;

    case 0x22: // Status
    case 0x26:
    case 0x2A:
    case 0x2E:
      data = 0x00;
      if (_client[(address >> 2) & 3]) {
        data |= 0x80;
        // if (_client[(address >> 2) & 3].overflow()) {
        //   data |= 0x10;
        // }
        if (_client[(address >> 2) & 3].availableForWrite()) {
          data |= 0x05;
        }
        if (_client[(address >> 2) & 3].available()) {
          data |= 0x02;
        }
      }
      break;

    // 70-7F: Card configuration
    case 0x7C: data = 0xC1; break; // Magic numbers for identification
    case 0x7E: data = 0x86; break;
  }
  return data;
}

void DeviceMulti::write(uint8_t address, uint8_t data) {
  switch (address) {

    // 00-0F: Disk
    case 0x00: // unit/secsz/status
      // Ignore write if busy
      if (_status & 0x10) { break; }
      _unit = data & 0x03;
      _secsz = 128 << ((data & 0x0C) >> 2);
      _status = _error = 0;
      break;

    case 0x02: // Sector count
      if (_status & 0x10) { break; }
      // Number needs to be written big-endian with any leading zeroes
      _secct <<= 8;
      _secct |= data;
      break;

    case 0x04: // Starting LBA
      if (_status & 0x10) { break; }
      _lba <<= 8;
      _lba |= data;
      break;

    case 0x06: // command/error/status
      if (_status & 0x10) { break; }
      _ptr = 0;
      _error = 0;
      _command = data;
      _status = 0x10;
      break;

    case 0x08: // Data
      // Write transfer in progress?
      if (_status & 0xA0 != 0x80) { break; }
      _secbuf[_ptr++] = data;
      // If finished, clear DRQ
      if (_ptr >= _secct * _secsz) { _status &= ~0x80; }
      break;

    // 10-1F: CLOCK$
    case 0x10:
      _update_dostime();
      break;

    // 20-2F: Character
    case 0x20:
    case 0x24:
    case 0x28:
    case 0x2C:
      if (_client[(address >> 2) & 3]) {
        _client[(address >> 2) & 3].write(data);
        // if (data < 0x20) { // ie. control characters
        //   _client[(address >> 2) & 3].flush();
        // }
      }
      break;

    // 70-7F: Card configuration
    // Nothing happens here on write
  }
}

void DeviceMulti::_update_dostime(void) {
  struct timeval tv;
  struct tm *ltime;
  uint16_t days;
  _dostimeptr = 0;
  gettimeofday(&tv, nullptr);
  ltime = localtime(&tv.tv_sec);

  _dostime[4] = tv.tv_usec / 10000;
  _dostime[5] = ltime->tm_sec;
  _dostime[2] = ltime->tm_min;
  _dostime[3] = ltime->tm_hour;

  days =
    _days_from_civil(ltime->tm_year + 1900, ltime->tm_mon + 1, ltime->tm_mday)
  - _days_from_civil(1980, 1, 1);
  _dostime[0] = days;
  _dostime[1] = days >> 8;
}

// From: https://howardhinnant.github.io/date_algorithms.html#days_from_civil
// Returns number of days since civil 1970-01-01.  Negative values indicate
//    days prior to 1970-01-01.
// Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
//                 m is in [1, 12]
//                 d is in [1, last_day_of_month(y, m)]
//                 y is "approximately" in
//                   [numeric_limits<Int>::min()/366, numeric_limits<Int>::max()/366]
//                 Exact range of validity is:
//                 [civil_from_days(numeric_limits<Int>::min()),
//                  civil_from_days(numeric_limits<Int>::max()-719468)]
template <class Int>
constexpr
Int
DeviceMulti::_days_from_civil(Int y, unsigned m, unsigned d) noexcept
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
             "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<Int>::digits >= 20,
             "This algorithm has not been ported to a 16 bit signed integer");
    y -= m <= 2;
    const Int era = (y >= 0 ? y : y-399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);      // [0, 399]
    const unsigned doy = (153*(m > 2 ? m-3 : m+9) + 2)/5 + d-1;  // [0, 365]
    const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]
    return era * 146097 + static_cast<Int>(doe) - 719468;
}
