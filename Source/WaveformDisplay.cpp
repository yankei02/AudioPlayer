/*
  ==============================================================================

    WaveformDisplay.cpp
    Created: 30 Sep 2024 9:53:18pm
    Author:  liann77

  ==============================================================================
*/

#include "WaveformDisplay.h"


//AudioThumbnail 是 JUCE 库中用于绘制音频波形的类。
WaveformDisplay::WaveformDisplay(juce::AudioThumbnail& thumbnailToUse)//造函数接收一个 juce::AudioThumbnail& 的引用，命名为 thumbnailToUse。
    : audioThumbnail(thumbnailToUse)//初始化成员变量 audioThumbnail：将传入的 thumbnailToUse 引用赋值给成员变量 audioThumbnail。
{
    audioThumbnail.addChangeListener(this);//将当前对象 (this) 添加为 audioThumbnail 的监听者。这意味着当 audioThumbnail 的状态发生变化时，会通知当前对象，以便更新显示。
}

WaveformDisplay::~WaveformDisplay()
{
    audioThumbnail.removeChangeListener(this);
}

void WaveformDisplay::paint(juce::Graphics& g)//如果 audioThumbnail 有有效的音频数据，则绘制波形；否则，显示提示文字。
{
    //背景颜色
    g.fillAll(juce::Colours::white);

    if (audioThumbnail.getTotalLength() > 0.0)
    {
        //绘制波形
        g.setColour(juce::Colours::blue);
        audioThumbnail.drawChannels(g, getLocalBounds(), 0.0, audioThumbnail.getTotalLength(), 1.0f);

        // 绘制播放位置线
        g.setColour(juce::Colours::red);
        auto audioPosition = currentPosition / audioThumbnail.getTotalLength();
        int x = static_cast<int>(audioPosition * getWidth());
        g.drawLine(x, 0, x, getHeight(), 2.0f);
    }
    else
    {
        //如果没有音频，显示文字
        g.setColour(juce::Colours::darkgrey);
        g.drawText("No Audio Loaded", getLocalBounds(), juce::Justification::centred, true);
    }
    // 绘制边框（可选，根据需要调整颜色和线宽）
    g.setColour(juce::Colours::pink);
    g.drawRect(getLocalBounds(),3);
}

void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster* source)//当 audioThumbnail 发生变化（例如加载了新的音频数据）时，调用此回调函数。检查通知源是否为 audioThumbnail，如果是，则调用 repaint()，请求组件重绘，以更新显示。
{
    if (source == &audioThumbnail)
    {
        repaint();
    }
}

void WaveformDisplay::setPosition(double position)//更新成员变量 currentPosition，表示当前音频的播放位置（以秒为单位）。调用 repaint()，请求组件重绘，以更新播放位置指示线。
{
    currentPosition = position;
    repaint();
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event)//点击，更新时间
{
    if (audioThumbnail.getTotalLength() > 0.0)
    {
        auto clickPosition = event.position.x / static_cast<float>(getWidth());
        auto newPosition = audioThumbnail.getTotalLength() * clickPosition;

        if (onPositionChanged)
            onPositionChanged(newPosition);
    }
}
