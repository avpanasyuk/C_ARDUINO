/**
 * @file SDP8xx.hpp
 * @author A. Panasyuk
 * @brief Sensirion SDP8xx-Digital differential-pressure sensor over Arduino Wire.
 *
 * Fully static (one sensor per program), no heap, no Arduino String -- matching the
 * house style of the C_ESP/C_General family. Reference: "Datasheet SDP8xx-Digital",
 * Sensirion, Version 1.1 - April 2019, archived on the packages share at
 * \\bsd\packages\DOCS\Electronics\DATASHEETS\SENSORS\
 * Sensirion_Differential_Pressure_Datasheet_SDP8xx_Digital.pdf
 *
 * Protocol in one paragraph: 16-bit commands, replies in 16-bit words each followed by a
 * CRC-8 byte (poly 0x31, init 0xFF, MSB-first -- avp::Crc8's defaults). After a
 * start-continuous command the sensor needs no further command: a bare I2C read header
 * returns DP, temperature and the DP scale factor (3 words, 9 bytes). Both raw values are
 * signed; physical = raw / scale factor, the DP scale factor coming from the sensor itself
 * (240 Pa^-1 on a -125Pa part) and the temperature one being a fixed 200 degC^-1.
 */
#pragma once

/// @cond
#include <stdint.h>
#include <Arduino.h>
#include <Wire.h>
/// @endcond
#include "C_General/Error.h"
#include "C_General/General.hpp"

namespace avp {
  class SDP8xx {
  public:
    /// SDP800/810 (both ranges) answer here; only SDP801/811-500Pa use 0x26.
    static constexpr uint8_t DefaultI2CAddr = 0x25;

    enum Cmd_t : uint16_t {
      ContMassFlowAvg = 0x3603,  ///< continuous, mass-flow compensated, averaged till read
      ContMassFlow = 0x3608,     ///< continuous, mass-flow compensated, no averaging
      ContDiffPressAvg = 0x3615, ///< continuous, DP compensated, averaged till read
      ContDiffPress = 0x361E,    ///< continuous, DP compensated, no averaging
      StopContinuous = 0x3FF9,   ///< back to idle; needed before ANY other command
      EnterSleep = 0x3677,       ///< from idle only; exit by addressing with a write bit
      ReadProductID1 = 0x367C,   ///< must be followed by ReadProductID2
      ReadProductID2 = 0xE102
    };

    /// Product numbers, top 24 bits only -- the datasheet marks the low byte as a revision
    /// that is "subject to change", so never compare all 32.
    static constexpr uint32_t PN_SDP810_125Pa = 0x03020B;
    static constexpr uint32_t PN_SDP800_125Pa = 0x030202;
    static constexpr uint32_t PN_SDP810_500Pa = 0x03020A;
    static constexpr uint32_t PN_SDP800_500Pa = 0x030201;

    /// Fixed by the datasheet; unlike the DP scale factor it is not transmitted.
    static constexpr float TemperatureScale = 200.f; // degC^-1

    /**
     * @brief Reset the sensor, identify it and put it in a continuous measurement mode.
     * @param SDA_pin,SCL_pin  the SDP8xx has no pull-ups of its own -- fit ~4.7k to 3.3V.
     * @param Freq_Hz  datasheet allows up to 1 MHz; 100 kHz is the safe default on flying leads.
     * @param StartCmd which continuous mode to leave the sensor in. ContDiffPressAvg averages
     *                 every sample taken since the previous read, so a slow poller gets the
     *                 true mean of the interval instead of an aliased instantaneous sample.
     * @return false if the sensor did not answer or failed CRC; @ref GetLastError says where.
     */
    static bool begin(int SDA_pin, int SCL_pin, uint32_t Freq_Hz = 100000,
      Cmd_t StartCmd = ContDiffPressAvg, uint8_t I2CAddr = DefaultI2CAddr) {
      Addr = I2CAddr;
      // ESP8266's Wire has no frequency overload (its begin() is begin(int,int)); ESP32's does.
      // setClock() after begin() is the portable spelling and does the same thing on both.
      Wire.begin(SDA_pin, SCL_pin);
      Wire.setClock(Freq_Hz);
      delay(PowerUpTime_ms);
      SoftReset();
      if(!ReadIdentifier()) return false;
      if(!WriteCmd(StartCmd)) {
        LastError = "start continuous failed";
        return false;
      }
      delay(FirstResultTime_ms);
      LastError = nullptr;
      return true;
    } // begin

    /**
     * @brief Read one DP + temperature + scale-factor triplet from a running continuous mode.
     * Do not call faster than every 0.5 ms (the sensor's own update interval).
     * @return false if the sensor NACKed (result not ready yet) or a CRC failed; the cached
     *         values are then left untouched.
     */
    static bool Read() {
      uint16_t Words[3];
      if(!ReadWords(Words, 3)) return false;
      ScaleFactor = (int16_t)Words[2];
      if(ScaleFactor == 0) {
        LastError = "zero scale factor";
        return false;
      }
      DP_Pa = (float)(int16_t)Words[0] / ScaleFactor;
      T_C = (float)(int16_t)Words[1] / TemperatureScale;
      ++NumReads;
      LastError = nullptr;
      return true;
    } // Read

    static float GetDP_Pa() { return DP_Pa; }
    static float GetT_C() { return T_C; }
    static int16_t GetScaleFactor() { return ScaleFactor; }
    static uint32_t GetProductNumber() { return ProductNumber; }
    static uint64_t GetSerialNumber() { return SerialNumber; }
    static uint32_t GetNumReads() { return NumReads; }
    /// nullptr when the last operation succeeded.
    static const char *GetLastError() { return LastError; }

    /// Human-readable part name from the product number, "unknown" for anything unlisted.
    static const char *GetPartName() {
      switch(ProductNumber >> 8) {
      case PN_SDP810_125Pa: return "SDP810-125Pa";
      case PN_SDP800_125Pa: return "SDP800-125Pa";
      case PN_SDP810_500Pa: return "SDP810-500Pa";
      case PN_SDP800_500Pa: return "SDP800-500Pa";
      default: return "unknown";
      }
    } // GetPartName

    /// General-call (address 0x00) reset -- the one command accepted while measuring.
    static void SoftReset() {
      Wire.beginTransmission(0x00);
      Wire.write(0x06);
      Wire.endTransmission();
      delay(SoftResetTime_ms);
    } // SoftReset

    static bool Stop() { return WriteCmd(StopContinuous); }

  private:
    /// Longest reply the sensor sends: the 6-word product identifier.
    static constexpr uint8_t MaxWordsPerRead = 6;
    static constexpr uint8_t PowerUpTime_ms = 25;     ///< datasheet t_PU, max
    static constexpr uint8_t SoftResetTime_ms = 5;    ///< datasheet t_SR max 2 ms, rounded up
    static constexpr uint8_t FirstResultTime_ms = 20; ///< 8 ms to first result + 12 ms settling

    static inline uint8_t Addr = DefaultI2CAddr;
    static inline int16_t ScaleFactor = 0;
    static inline float DP_Pa = 0.f, T_C = 0.f;
    static inline uint32_t ProductNumber = 0, NumReads = 0;
    static inline uint64_t SerialNumber = 0;
    static inline const char *LastError = "not started";

    static bool WriteCmd(uint16_t Cmd) {
      Wire.beginTransmission(Addr);
      Wire.write((uint8_t)(Cmd >> 8));
      Wire.write((uint8_t)Cmd);
      return Wire.endTransmission() == 0;
    } // WriteCmd

    /**
     * @brief Read @p NumWords 16-bit words, each validated against its trailing CRC byte.
     * A NACK here is normal, not a fault: the sensor NACKs the read header whenever no
     * result is ready yet.
     */
    static bool ReadWords(uint16_t *Words, uint8_t NumWords) {
      const uint8_t NumBytes = NumWords * 3;
      uint8_t Buf[3 * MaxWordsPerRead];
      // A real runtime check, not AVP_ASSERT: release builds define NDEBUG, under which
      // AVP_ASSERT evaluates its expression and discards the result (Error.h), so it would
      // guard nothing in the binary that actually gets flashed.
      if(NumBytes > sizeof(Buf)) {
        LastError = "read longer than the buffer";
        return false;
      }
      // != NumBytes is right on both cores, for different reasons: ESP8266's requestFrom is
      // all-or-nothing (returns size, or 0 on failure), ESP32's returns the count actually
      // received. Either way a short read is a failure here -- the frame is fixed-length.
      if(Wire.requestFrom((int)Addr, (int)NumBytes) != NumBytes) {
        LastError = "no answer (result not ready?)";
        return false;
      }
      for(uint8_t i = 0; i < NumBytes; ++i) Buf[i] = Wire.read();
      for(uint8_t w = 0; w < NumWords; ++w) {
        const uint8_t *p = Buf + 3 * w;
        if(avp::Crc8(p, 2) != p[2]) {
          LastError = "CRC error";
          return false;
        }
        Words[w] = ((uint16_t)p[0] << 8) | p[1];
      }
      return true;
    } // ReadWords

    /// 0x367C + 0xE102 then 6 words: 2 of product number, 4 of serial number.
    static bool ReadIdentifier() {
      if(!WriteCmd(ReadProductID1) || !WriteCmd(ReadProductID2)) {
        LastError = "sensor does not answer";
        return false;
      }
      uint16_t Words[6];
      if(!ReadWords(Words, 6)) return false;
      ProductNumber = ((uint32_t)Words[0] << 16) | Words[1];
      SerialNumber = 0;
      for(uint8_t i = 2; i < 6; ++i) SerialNumber = (SerialNumber << 16) | Words[i];
      return true;
    } // ReadIdentifier
  }; // class SDP8xx
} // namespace avp
