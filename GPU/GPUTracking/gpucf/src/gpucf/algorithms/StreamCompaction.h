#pragma once

#include <gpucf/common/Event.h>
#include <gpucf/common/Fragment.h>
#include <gpucf/common/Measurements.h>

#include <CL/cl2.hpp>

#include <stack>
#include <vector>


namespace gpucf
{

class ClEnv;


class StreamCompaction
{

private:
    struct DeviceMemory
    {
        std::vector<cl::Buffer> incrBufs;
        std::vector<size_t>     incrBufSizes;
    };

public:
    class Worker
    {

    public:
        Worker(const Worker &) = default;

        size_t run(
                const Fragment &,
                cl::CommandQueue,
                cl::Buffer,
                cl::Buffer,
                cl::Buffer,
                bool debug=false);

        std::vector<std::vector<int>> getNewIdxDump() const;

        Step asStep(const std::string &) const;

    private:
        friend class StreamCompaction;

        cl::Kernel nativeScanUpStart;
        size_t     scanUpStartWorkGroupSize;

        cl::Kernel nativeScanUp;
        size_t     scanUpWorkGroupSize;

        cl::Kernel nativeScanTop;
        size_t     scanTopWorkGroupSize;

        cl::Kernel nativeScanDown;
        cl::Kernel compactArr;

        DeviceMemory mem;

        std::vector<std::vector<int>> sumsDump;

        std::vector<Event> scanEvents;
        Event compactArrEv;
        Event readNewDigitNum;

        cl::Event *addScanEvent();

        size_t stepnum(const Fragment &) const;

        Worker(cl::Program, cl::Device, DeviceMemory);

        void dumpBuffer(cl::CommandQueue, cl::Buffer, size_t);

    };

    void setup(ClEnv &, size_t, size_t); 

    void setDigitNum(size_t, size_t);

    Worker worker();


private:
    cl::Context context;
    cl::Device  device;

    nonstd::optional<cl::Program> prg;

    std::stack<DeviceMemory> mems;

    size_t workernum = 0;

    size_t digitNum = 0;

};

} // namespace gpucf

// vim: set ts=4 sw=4 sts=4 expandtab: