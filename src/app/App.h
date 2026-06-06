#pragma once

class App
{
public:
    bool Initialize();
    void RunFrame();
    void Shutdown();

    bool ShouldClose() const;

private:
    bool m_shouldClose = false;
    int m_frameIndex = 0;
};
