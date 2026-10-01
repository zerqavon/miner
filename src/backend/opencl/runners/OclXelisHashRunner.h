#ifndef XMRIG_OCLXELISHASHRUNNER_H
#define XMRIG_OCLXELISHASHRUNNER_H

#include "backend/opencl/runners/OclBaseRunner.h"

namespace xmrig {

class OclXelisHashRunner final : public OclBaseRunner
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(OclXelisHashRunner)
    OclXelisHashRunner(size_t index, const OclLaunchData &data);
    ~OclXelisHashRunner() override;

protected:
    void run(uint32_t nonce, uint32_t nonce_offset, uint32_t *hashOutput) override;
    void set(const Job &job, uint8_t *blob) override;
    void build() override;
    void init() override;
    uint32_t roundSize() const override { return m_batch; }
    uint32_t processedHashes() const override { return m_batch; }

private:
    cl_kernel m_kernel = nullptr;
    cl_mem m_hashes = nullptr;
    cl_mem m_scratch = nullptr;
    cl_mem m_target = nullptr;
    cl_mem m_solutions = nullptr;
    cl_mem m_lut = nullptr;
    uint32_t m_batch = 32;
    uint32_t m_workGroupSize = 1;
};

} // namespace xmrig

#endif
