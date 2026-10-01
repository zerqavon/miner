/* LiquidMiner
 * Generic GPU profile generator for non-RandomX GPU PoW algorithms.
 */

#include "backend/opencl/OclThreads.h"
#include "backend/opencl/wrappers/OclDevice.h"
#include "base/crypto/Algorithm.h"


namespace xmrig {


bool ocl_generic_gpupow_generator(const OclDevice &device, const Algorithm &algorithm, OclThreads &threads)
{
    const auto family = algorithm.family();

    if (family != Algorithm::XELISHASH_FAMILY &&
        family != Algorithm::NEXAPOW_FAMILY &&
        family != Algorithm::OGGPOW_FAMILY) {
        return false;
    }

    bool isModernNavi = false;
    bool isNavi = false;

    switch (device.type()) {
    case OclDevice::Navi_10:
    case OclDevice::Navi_12:
    case OclDevice::Navi_14:
    case OclDevice::Navi_21:
        isNavi = true;
        break;

    case OclDevice::Navi_31:
    case OclDevice::Navi_32:
    case OclDevice::Navi_33:
    case OclDevice::Navi_44:
    case OclDevice::Navi_48:
        isNavi = true;
        isModernNavi = true;
        break;

    default:
        break;
    }

    const uint32_t worksize = isModernNavi ? 256 : (isNavi ? 128 : 256);
    const uint32_t cuIntensity = isModernNavi ? 524288 : 262144;
    const uint32_t effectiveComputeUnits = device.computeUnits() * (isModernNavi ? 2 : 1);

    threads.add(OclThread(device.index(), effectiveComputeUnits * cuIntensity, worksize, 1));

    return true;
}


} // namespace xmrig
