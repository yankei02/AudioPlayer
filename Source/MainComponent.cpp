#include "MainComponent.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <cairo/cairo.h>
#include <poppler/glib/poppler.h>  // Poppler C API
#include <glib.h>                  // GLib 头文件，用于 GError 等类型
#include "WaveformDisplay.h"
#include "MarkerSlider.h"
#include "Marker.h"
#include <fontconfig/fontconfig.h>

//==============================================================================


MainComponent::MainComponent()
:thumbnailCache(10),  // 初始化thumbnailCache，缓存大小为 10
audioThumbnail(512, formatManager, thumbnailCache),  // 初始化 audioThumbnail
waveformDisplay(audioThumbnail),currentPageIndex(0),// 初始化 currentPageIndex 为 0
totalNumPages(0),    // 初始化 totalNumPages 为 0
pdfDocFileName(""),// 初始化 pdfDocFileName 为空字符串
grayLookAndFeel()

{
    // 确保设备管理器已初始化，使用默认设置
    deviceManager.initialise(0, 2, nullptr, true);
    setAudioChannels(0, 2);  // 设置音频输入输出通道

    // 注册音频格式管理器
    formatManager.registerBasicFormats();

    // 添加控件
    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(progressSlider);
    addAndMakeVisible(audioFileNameLabel);
    addAndMakeVisible(pdfFileNameLabel);  // 添加 PDF 文件名的标签
    addAndMakeVisible(playButton);
    addAndMakeVisible(pauseButton);
    addAndMakeVisible(pdfImageComponent); // 添加 PDF 显示区域
    addAndMakeVisible(nextPagePreview);   // 添加下一页预览组件
    addAndMakeVisible(nextButton);
    addAndMakeVisible(beforeButton);
    addAndMakeVisible(audioPositionLabel);
    addAndMakeVisible(audioLengthLabel);
    // 添加和配置新的 markerSlider
    addAndMakeVisible(markerSlider);

    // 设置组件可见
    audioFileNameLabel.setVisible(false);
    pdfFileNameLabel.setVisible(false);
    pdfImageComponent.setVisible(false);
    nextPagePreview.setVisible(false);
    audioPositionLabel.setVisible(true);  // 初始设置为不可见
    audioLengthLabel.setVisible(true);
    waveformDisplay.setVisible(true);
    

    
    // 设置按钮回调
    playButton.onClick = [this]
    {
        DBG("Play button clicked");
        if (readerSource.get() != nullptr)
        {
            transportSource.start();  // 确保设置了音频源后再启动播放
            DBG("Audio started playing");
        }
    };
    pauseButton.onClick = [this] { transportSource.stop(); };
    nextButton.onClick = [this]
    {
        // 处理翻到下一页
        if (pdfDoc && currentPageIndex + 1 < totalNumPages)
        {
            ++currentPageIndex;

            // 加载并显示当前页
            PopplerPage* pdfPage = poppler_document_get_page(pdfDoc, currentPageIndex);
            if (pdfPage)
            {
                renderPdfPageToComponent(pdfPage, pdfImageComponent, currentPageIndex);
                g_object_unref(pdfPage);
            }
            else
            {
                DBG("Failed to load page " + juce::String(currentPageIndex));
                return;
            }

            // 加载并显示下一页预览
            if (currentPageIndex + 1 < totalNumPages)
            {
                PopplerPage* nextPdfPage = poppler_document_get_page(pdfDoc, currentPageIndex + 1);
                if (nextPdfPage)
                {
                    renderPdfPageToComponent(nextPdfPage, nextPagePreview, nextPageIndex);
                    g_object_unref(nextPdfPage);
                    nextPagePreview.setVisible(true);
                }
                else
                {
                    nextPagePreview.setVisible(false);
                }
            }
            else
            {
                nextPagePreview.setVisible(false);
            }

            // 更新 PDF 文件名标签，显示当前页码
            pdfFileNameLabel.setText("PDF: " + pdfDocFileName + " (Page " + juce::String(currentPageIndex + 1) + "/" + juce::String(totalNumPages) + ")", juce::dontSendNotification);

            // 更新按钮的启用状态
            beforeButton.setEnabled(currentPageIndex > 0);
            nextButton.setEnabled(currentPageIndex + 1 < totalNumPages);

            repaint();
        }
    };


    beforeButton.onClick = [this]
        {
            // 处理返回上一页
            if (pdfDoc && currentPageIndex > 0)
            {
                --currentPageIndex;

                // 加载并显示当前页
                PopplerPage* pdfPage = poppler_document_get_page(pdfDoc, currentPageIndex);
                if (pdfPage)
                {
                    renderPdfPageToComponent(pdfPage, pdfImageComponent, currentPageIndex);
                    g_object_unref(pdfPage);
                }
                else
                {
                    DBG("Failed to load page " + juce::String(currentPageIndex));
                    return;
                }

                // 加载并显示下一页预览
                if (currentPageIndex + 1 < totalNumPages)
                {
                    PopplerPage* nextPdfPage = poppler_document_get_page(pdfDoc, currentPageIndex + 1);
                    if (nextPdfPage)
                    {
                        renderPdfPageToComponent(nextPdfPage, nextPagePreview, nextPageIndex);
                        g_object_unref(nextPdfPage);
                        nextPagePreview.setVisible(true);
                    }
                    else
                    {
                        nextPagePreview.setVisible(false);
                    }
                }
                else
                {
                    nextPagePreview.setVisible(false);
                }

                // 更新 PDF 文件名标签，显示当前页码
                pdfFileNameLabel.setText("PDF: " + pdfDocFileName + " (Page " + juce::String(currentPageIndex + 1) + "/" + juce::String(totalNumPages) + ")", juce::dontSendNotification);

                // 更新按钮的启用状态
                beforeButton.setEnabled(currentPageIndex > 0);
                nextButton.setEnabled(currentPageIndex + 1 < totalNumPages);

                repaint();
            }
        };

    // 设置定时器，用于更新播放进度
    startTimer(500);  // 每半秒更新一次进度条
    progressSlider.setRange(0.0, 1.0);  // 进度条的范围从 0 到 1
    // 设置 progressSlider 的 LookAndFeel
    progressSlider.setLookAndFeel(&grayLookAndFeel);

    // 设置进度条的样式
    progressSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    progressSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    // 设置进度条颜色
    progressSlider.setColour(juce::Slider::thumbColourId, juce::Colours::grey);
    progressSlider.setColour(juce::Slider::trackColourId, juce::Colours::grey);
    progressSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    //pdf名称的颜色
    pdfFileNameLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    audioFileNameLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    //设置slider旁边秒数的颜色
    audioPositionLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    audioLengthLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    // 设置按钮初始状态
    beforeButton.setEnabled(false);
    nextButton.setEnabled(false);

    progressSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    //更新call back
    waveformDisplay.onPositionChanged = [this](double newPosition)
        {
            transportSource.setPosition(newPosition);//设置音频播放位置
            waveformDisplay.setPosition(newPosition); // 设置波形显示位置
        };
    // 设置 AudioTransportSource 的监听器
    //transportSource.addChangeListener(this);
    //确保在 progressSlider 的 onValueChange 回调中同步更新 transportSource 和 waveformDisplay 的位置。
    progressSlider.onValueChange = [this]()
    {
        double newPosition = progressSlider.getValue();
        transportSource.setPosition(newPosition);       // 设置音频播放位置
        waveformDisplay.setPosition(newPosition);       // 设置波形显示位置
    };
    //设置markerSlider 长度与audioLength一样
    double audioLength = audioThumbnail.getTotalLength();
    if (audioLength > 0.0)
    {
        markerSlider.setRange(0.0, audioLength, 0.001);
    }
    else
    {
        markerSlider.setRange(0.0, 1.0); // 设置默认范围，避免断言失败
    }

    markerSlider.setSliderStyle(juce::Slider::LinearHorizontal); // 设置为线性水平滑块
    markerSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // 无文本框
    markerSlider.setLookAndFeel(&grayLookAndFeel); // 使用自定义 LookAndFeel

    // 设置标记改变后的回调
    markerSlider.onMarkersChanged = [this]()
    {
        int lastDraggedIndex = markerSlider.getLastDraggedMarkerIndex();
        if (lastDraggedIndex >= 0 && lastDraggedIndex < static_cast<int>(markerSlider.getMarkers().size()))
        {
            const Marker& draggedMarker = markerSlider.getMarkers()[lastDraggedIndex];
            markerTriggeredFlags[&draggedMarker] = false;  // 重置被拖动标记的触发状态
        }
    };
    // 添加保存标记按钮
    addAndMakeVisible(saveMarkersButton);
    // 使用 juce::CharPointer_UTF8 包装 UTF-8 字符串
    saveMarkersButton.setButtonText("SaveMarkers");
    saveMarkersButton.onClick = [this]()
    {
        DBG("Save Markers button clicked");

        auto fileChooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser = std::make_unique<juce::FileChooser>(
            juce::String(juce::CharPointer_UTF8("保存标记位置")),
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
            "*.markers");

        // 使用 SafePointer 捕获 this 指针
        juce::Component::SafePointer<MainComponent> safeThis(this);

        fileChooser->launchAsync(fileChooserFlags, [safeThis](const juce::FileChooser& fc)
        {
            if (auto* strongThis = safeThis.getComponent())
            {
                juce::File file = fc.getResult();
                if (file != juce::File{})
                {
                    DBG("File selected: " + file.getFullPathName());

                    strongThis->saveMarkerPositions(file);
                    DBG("Marker positions saved to: " + file.getFullPathName());
                }
                else
                {
                    DBG("User cancelled the file save operation.");
                }

                // 重置 fileChooser，释放资源
                strongThis->fileChooser.reset();
            }
        });
    };

    // 设置组件大小
    setSize(800,600);

}

MainComponent::~MainComponent()
{
    markerSlider.setLookAndFeel(nullptr); // 解除 LookAndFeel 绑定
    progressSlider.setLookAndFeel(nullptr);  // 解除 LookAndFeel 的绑定
    // 停止播放并释放资源
    transportSource.stop();
    transportSource.setSource(nullptr);
    shutdownAudio(); // 确保在基类析构之前调用
}

void MainComponent::timerCallback()
{
    if (transportSource.isPlaying())
    {
        // 获取当前播放位置和音频总时长
        double position = transportSource.getCurrentPosition();
        double length = transportSource.getLengthInSeconds();

        // 更新进度条的值为当前播放位置
        progressSlider.setValue(position, juce::dontSendNotification);

        // 更新当前播放时间标签
        int currentMinutes = static_cast<int>(position) / 60;
        int currentSeconds = static_cast<int>(position) % 60;
        juce::String positionText = juce::String::formatted("%02d:%02d", currentMinutes, currentSeconds);
        audioPositionLabel.setText(positionText, juce::dontSendNotification);
        audioPositionLabel.setVisible(true);  // 音频播放时显示标签

        // 更新音频总时长标签
        int totalMinutes = static_cast<int>(length) / 60;
        int totalSeconds = static_cast<int>(length) % 60;
        juce::String lengthText = juce::String::formatted("%02d:%02d", totalMinutes, totalSeconds);
        audioLengthLabel.setText(lengthText, juce::dontSendNotification);
        audioLengthLabel.setVisible(true);

        // 更新波形显示的位置
        waveformDisplay.setPosition(position);
        // 检查 markerSlider 上的标记是否被触发
        for (const auto& marker : markerSlider.getMarkers())
        {
            // 计算标记对应的时间
            double markerTime = marker.position; // marker.position 是以秒为单位

            // 如果当前播放位置超过标记时间且标记尚未被触发
            if (!markerTriggeredFlags[&marker] && position >= markerTime)
            {
                markerTriggeredFlags[&marker] = true; // 标记为已触发
                nextButton.triggerClick(); // 触发下一页
                DBG("Marker reached at position: " + juce::String(markerTime));
            }
        }
    }
    else
    {
        // 音频未播放时显示当前位置
        audioPositionLabel.setVisible(true);
    }
}


//process block
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}


void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
    }
    else
    {
        transportSource.getNextAudioBlock(bufferToFill);
    }
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}
void MainComponent::saveMarkerPositions(const juce::File& file)
{
    if (file == juce::File{})
    {
        DBG("Invalid file provided for saving marker positions.");
        return;
    }

    juce::FileOutputStream outputStream(file);

    if (!outputStream.openedOk())
    {
        DBG("Failed to open file for writing: " + file.getFullPathName());
        return;
    }

    // 获取所有标记
    const auto& markers = markerSlider.getMarkers();

    // 将每个标记的位置写入文件，每个位置占一行
    for (const auto& marker : markers)
    {
        outputStream.writeText(juce::String(marker.position) + "\n", false, false, "\n");
    }

    outputStream.flush();
}


//marker load
void MainComponent::loadMarkerPositions(const juce::File& file)
{
    if (file == juce::File{} || !file.existsAsFile())
    {
        DBG("Invalid or non-existent file provided for loading marker positions.");
        return;
    }

    juce::FileInputStream inputStream(file);

    if (!inputStream.openedOk())
    {
        DBG("Failed to open file for reading: " + file.getFullPathName());
        return;
    }

    // 清除当前的标记
    markerSlider.clearMarkers();
    markerTriggeredFlags.clear();

    // 读取文件中的每一行，解析标记位置
    while (!inputStream.isExhausted())
    {
        juce::String line = inputStream.readNextLine();
        double position = line.getDoubleValue();

        if (position >= markerSlider.getMinimum() && position <= markerSlider.getMaximum())
        {
            markerSlider.addMarker(position);
        }
        else
        {
            DBG("Marker position out of range: " + line);
        }
    }

    // 初始化触发标志
    for (const auto& marker : markerSlider.getMarkers())
    {
        markerTriggeredFlags[&marker] = false;
    }
}



//拖拽文件，仅对音频以及pdf文件感兴趣
bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    const juce::File file(files[0]);
    return file.hasFileExtension(".wav") || file.hasFileExtension(".mp3") || file.hasFileExtension(".pdf") || file.hasFileExtension(".markers");
}

void MainComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    auto file = juce::File(files[0]);
    DBG("File dropped: " + file.getFileName());

    if (file.hasFileExtension(".wav") || file.hasFileExtension(".mp3"))
    {
        // 处理音频文件
        auto* reader = formatManager.createReaderFor(file);
        if (reader != nullptr)
        {
            std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
            transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
            readerSource.reset(newSource.release());
            transportSource.start();  // 开始播放音频
            audioFileNameLabel.setText("Audio: " + file.getFileName(), juce::dontSendNotification);
            audioFileNameLabel.setVisible(true);
            // 加载音频文件到 AudioThumbnail
            audioThumbnail.clear();
            audioThumbnail.setSource(new juce::FileInputSource(file));
            // 设置 progressSlider 的范围为音频总时长
            double audioLength = audioThumbnail.getTotalLength();
            // 检查音频长度是否有效
            if (audioLength > 0.0)
            {
                progressSlider.setRange(0.0, audioLength, 0.1);  // 设置范围为 0 到音频总时长，步进为 0.1 秒
                progressSlider.setValue(0.0, juce::dontSendNotification);  // 初始值设为 0

                // 同步更新 markerSlider 的范围
                markerSlider.setRange(0.0, audioLength, 0.1); // 设置 markerSlider 的范围与 progressSlider 一致
            }
            else
            {
                DBG("Audio length is invalid: " + juce::String(audioLength));
                // 设置一个默认范围，避免断言失败
                progressSlider.setRange(0.0, 1.0, 0.1);
                markerSlider.setRange(0.0, 1.0, 0.1);
            }
            // 在设置新的滑块范围后，重新添加标记
            recalculateAndAddMarkers();
            // 重置 markerTriggeredFlags
            for (auto& [marker, triggered] : markerTriggeredFlags)
            {
                triggered = false;
            }
        }
    }
    else if (file.hasFileExtension(".pdf"))
    {
        // 使用 Poppler 处理 PDF 文件
        loadAndDisplayPDF(file);
        pdfFileNameLabel.setText("PDF: " + file.getFileName(), juce::dontSendNotification);
        pdfFileNameLabel.setVisible(true);

        // 设置字体颜色为粉色
        pdfFileNameLabel.setColour(juce::Label::textColourId, juce::Colours::pink);
        
    }
    else if (file.hasFileExtension(".markers"))
    {
        // 检查是否已加载音频文件
        if (readerSource.get() == nullptr)
        {
            DBG("Load audio file before load marker file");
            // 可以在界面上显示提示
            return;
        }

        // 处理标记文件
        loadMarkerPositions(file);
        DBG("Marker positions loaded from: " + file.getFullPathName());
    }

}

void MainComponent::recalculateAndAddMarkers()
{
    // 清除现有标记
    markerSlider.clearMarkers();
    markerTriggeredFlags.clear();

    // 如果 totalNumPages 小于等于 1，则不添加标记
    if (totalNumPages <= 1)
        return;

    double sliderMin = markerSlider.getMinimum();
    double sliderMax = markerSlider.getMaximum();
    double sliderRange = sliderMax - sliderMin;

    // 使用音频长度计算标记位置
    for (int i = 1; i < totalNumPages; ++i)
    {
        double markerPosition = sliderMin + (static_cast<double>(i) / totalNumPages) * sliderRange;
        markerSlider.addMarker(markerPosition);
    }

    // 初始化触发标志
    for (const auto& marker : markerSlider.getMarkers())
    {
        markerTriggeredFlags[&marker] = false;
    }
}


//需要手动指定删除器
struct PopplerDocumentDeleter
{
    void operator()(poppler::document* doc) const
    {
        delete doc;  // 释放 document 指针
    }
};

struct PopplerPageDeleter
{
    void operator()(poppler::page* page) const
    {
        delete page;  // 释放 page 指针
    }
};

void MainComponent::loadAndDisplayPDF(const juce::File& pdfFile)
{
    // 使用 juce::URL::addEscapeChars 对文件路径进行编码
    juce::String filePath = pdfFile.getFullPathName();
    juce::String escapedFilePath = juce::URL::addEscapeChars(filePath, "");

    // 构建正确的文件 URI
    juce::String fileURI = juce::URL(pdfFile).toString(true);

    // 释放之前的 PDF 文档
    if (pdfDoc)
    {
        g_object_unref(pdfDoc);
        pdfDoc = nullptr;
    }

    // 使用 Poppler C API 加载 PDF 文档
    GError* gerror = nullptr;
    pdfDoc = poppler_document_new_from_file(fileURI.toRawUTF8(), nullptr, &gerror);

    if (!pdfDoc)
    {
        juce::String errorMessage = "Failed to load PDF file.";
        if (gerror != nullptr && gerror->message != nullptr)
        {
            errorMessage += " Error: " + juce::String(gerror->message);
        }
        else
        {
            errorMessage += " Unknown error.";
        }

        DBG(errorMessage);

        if (gerror != nullptr)
        {
            g_error_free(gerror);
        }

        return;
    }


    // 存储 PDF 文件名
    pdfDocFileName = pdfFile.getFileName();

    // 初始化页码
    currentPageIndex = 0;
    totalNumPages = poppler_document_get_n_pages(pdfDoc);

    // 加载并显示当前页
    PopplerPage* pdfPage = poppler_document_get_page(pdfDoc, currentPageIndex);
    if (pdfPage)
    {
        renderPdfPageToComponent(pdfPage, pdfImageComponent, currentPageIndex);
        g_object_unref(pdfPage);
    }
    else
    {
        DBG("Failed to load page " + juce::String(currentPageIndex));
        g_object_unref(pdfDoc);
        pdfDoc = nullptr;
        return;
    }

    // 加载并显示下一页预览
    if (currentPageIndex + 1 < totalNumPages)
    {
        PopplerPage* nextPdfPage = poppler_document_get_page(pdfDoc, currentPageIndex + 1);
        if (nextPdfPage)
        {
            renderPdfPageToComponent(nextPdfPage, nextPagePreview, nextPageIndex);
            g_object_unref(nextPdfPage);
            nextPagePreview.setVisible(true);
        }
    }
    else
    {
        nextPagePreview.setVisible(false);
    }

    // 更新 PDF 文件名标签
    pdfFileNameLabel.setText("PDF: " + pdfDocFileName + " (Page " + juce::String(currentPageIndex + 1) + "/" + juce::String(totalNumPages) + ")", juce::dontSendNotification);
    pdfFileNameLabel.setVisible(true);

    // 调用 recalculateAndAddMarkers 来添加标记
    recalculateAndAddMarkers();
    /*// 清除现有标记
    markerSlider.clearMarkers();
    markerTriggeredFlags.clear();
    if (totalNumPages > 1)
    {
        // 获取滑块的最小值和最大值
        double sliderMin = markerSlider.getMinimum();
        double sliderMax = markerSlider.getMaximum();
        double sliderRange = sliderMax - sliderMin;

        for (int i = 1; i < totalNumPages; ++i)
        {
            // 计算标记的位置，基于滑块的范围（0 到 1）
            double markerPosition = sliderMin + (static_cast<double>(i) / totalNumPages) * sliderRange;
            markerSlider.addMarker(markerPosition);
        }
    

        // 初始化触发标志
        for (const auto& marker : markerSlider.getMarkers())
        {
            markerTriggeredFlags[&marker] = false;
        }
    }*/

    // 根据总页数启用或禁用 Next 按钮
    nextButton.setEnabled(currentPageIndex + 1 < totalNumPages);
    beforeButton.setEnabled(currentPageIndex > 0);
      
    // 确保 PDF 显示区域可见
    pdfImageComponent.setVisible(true);
    repaint();
    // 清空缓存
    renderedPageCache.clear();

}

void MainComponent::renderPdfPageToComponent(PopplerPage* pdfPage, juce::ImageComponent& component, int pageIndex)
{
    // 检查缓存中是否已有该页面的图像
    auto it = renderedPageCache.find(pageIndex);
    if (it != renderedPageCache.end())
    {
        // 使用缓存的图像
        component.setImage(it->second);
        return;
    }

    // 获取组件的尺寸
    const int targetWidth = component.getWidth();
    const int targetHeight = component.getHeight();

    // 获取 PDF 页面尺寸（以点为单位，1点=1/72英寸）
    double pdfPageWidthPoints, pdfPageHeightPoints;
    poppler_page_get_size(pdfPage, &pdfPageWidthPoints, &pdfPageHeightPoints);

    // 计算 PDF 页面宽高比
    double pdfAspectRatio = pdfPageWidthPoints / pdfPageHeightPoints;
    double componentAspectRatio = static_cast<double>(targetWidth) / targetHeight;

    // 根据组件尺寸和 PDF 页面比例，计算渲染尺寸
    int renderWidth, renderHeight;
    if (pdfAspectRatio > componentAspectRatio)
    {
        // PDF 更宽，以组件宽度为基准
        renderWidth = targetWidth;
        renderHeight = static_cast<int>(renderWidth / pdfAspectRatio);
    }
    else
    {
        // PDF 更高，以组件高度为基准
        renderHeight = targetHeight;
        renderWidth = static_cast<int>(renderHeight * pdfAspectRatio);
    }

    // 设置目标 DPI
    double targetDPI = (renderWidth * 144.0) / pdfPageWidthPoints;

    // 创建 Cairo Surface
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, renderWidth, renderHeight);
    cairo_t* cr = cairo_create(surface);

    // 设置抗锯齿
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // 设置缩放比例
    double scale = targetDPI / 144.0;
    cairo_scale(cr, scale, scale);

    // 渲染 PDF 页面到 Cairo Surface
    poppler_page_render(pdfPage, cr);

    // 读取 Cairo Surface 数据并将其转换为 JUCE 图像
    unsigned char* data = cairo_image_surface_get_data(surface);
    cairo_surface_flush(surface);  // 确保数据已刷新

    juce::Image juceImage(juce::Image::ARGB, renderWidth, renderHeight, false);

    // 获取每行的字节数（步幅）
    int stride = cairo_image_surface_get_stride(surface);

    // 复制像素数据，处理颜色通道和预乘 Alpha
    for (int y = 0; y < renderHeight; ++y)
    {
        for (int x = 0; x < renderWidth; ++x)
        {
            int offset = y * stride + x * 4;

            // 读取 32 位像素值
            uint32_t pixel = *reinterpret_cast<uint32_t*>(data + offset);

            // 提取颜色通道，假设系统是小端字节序（常见于 x86 架构）
            juce::uint8 alpha = (pixel >> 24) & 0xFF;
            juce::uint8 red   = (pixel >> 16) & 0xFF;
            juce::uint8 green = (pixel >> 8)  & 0xFF;
            juce::uint8 blue  = pixel & 0xFF;

            // 处理预乘 Alpha（将颜色值除以 Alpha 值）
            if (alpha != 0)
            {
                red   = static_cast<juce::uint8>(std::min((red * 255) / alpha, 255));
                green = static_cast<juce::uint8>(std::min((green * 255) / alpha, 255));
                blue  = static_cast<juce::uint8>(std::min((blue * 255) / alpha, 255));
            }

            // 设置像素到 JUCE 图像
            juceImage.setPixelAt(x, y, juce::Colour::fromRGBA(red, green, blue, alpha));
        }
    }

    // 在组件中显示图像，不进行缩放
    component.setImage(juceImage);

    // 将图像存入缓存
    renderedPageCache[pageIndex] = juceImage;

    // 清理 Cairo 资源
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

//==============================================================================


void MainComponent::paint(juce::Graphics& g)
{
    // 设置白色背景
    g.fillAll(juce::Colours::white);

    // 如果没有 PDF 文件，显示提示信息
    if (!pdfImageComponent.isVisible())
    {
        g.setColour(juce::Colours::lightgrey);
        g.fillRect(pdfImageComponent.getBounds());  // 灰色填充
        g.setColour(juce::Colours::black);
        g.drawText("Please drag PDF here", pdfImageComponent.getBounds(), juce::Justification::centred);
    }

    // 绘制边框
    g.setColour(juce::Colours::pink);
    g.drawRect(pdfImageComponent.getBounds(), 2);  // 大的 PDF 显示区域，粉色边框

    g.setColour(juce::Colours::pink);
    g.drawRect(nextPagePreview.getBounds(), 2);  // 小的下一页预览区域，粉色边框

    // 绘制文件名显示区域的边框
    g.setColour(juce::Colours::black);
    g.drawRect(audioFileNameLabel.getBounds(), 2);  // 音频文件名的边框
    g.drawRect(pdfFileNameLabel.getBounds(), 2);    // PDF 文件名的边框
    
    //
    
}

void MainComponent::resized()
{
    // 定义边距和间距
    int margin = 10;
    int spacing = 10;

    // 定义组件大小
    int buttonWidth = 60;
    int buttonHeight = 25;
    int sliderHeight = 20;
    int labelHeight = 20;

    // 计算可用宽度和高度
    int availableWidth = getWidth() - margin * 2;
    int availableHeight = getHeight() - margin * 6 - buttonHeight - sliderHeight - labelHeight - spacing * 5;

    // 计算横向 A4 的宽高比（宽 / 高）
    double a4AspectRatio = 297.0 / 210.0; // ≈1.414

    // 设置主 PDF 显示区域的大小
    // 以 availableWidth 的 2/3 为基础，计算高度，使其符合 A4 横向比例
    int pdfWidth = availableWidth * 2 / 3; // 主 PDF 框占据可用宽度的 2/3
    int pdfHeight = static_cast<int>(pdfWidth / a4AspectRatio);

    // 如果计算的高度超过 availableHeight，则需要缩小宽度和高度
    if (pdfHeight > availableHeight)
    {
        pdfHeight = availableHeight;
        pdfWidth = static_cast<int>(pdfHeight * a4AspectRatio);
    }

    // 设置主 PDF 框的位置（左侧）
    int pdfX = margin;
    int pdfY = margin * 3;

    pdfImageComponent.setBounds(pdfX, pdfY, pdfWidth, pdfHeight);

    // 调整 PDF 文件名标签的大小
    int pdfLabelWidth = 100; // 调整为较小的宽度
    int pdfLabelHeight = 15; // 调整高度

    // 计算总控件宽度：两个按钮、标签和两个间距
    int totalControlsWidth = buttonWidth * 2 + pdfLabelWidth + spacing * 2;

    // 计算控件的起始 X 坐标，使其居中
    int controlsX = pdfX + (pdfWidth - totalControlsWidth) / 2;
    int controlsY = pdfImageComponent.getBottom() + spacing;

    // 设置 Before 按钮的位置（左侧）
    beforeButton.setBounds(controlsX, controlsY, buttonWidth, buttonHeight);

    // 设置 PDF 文件名标签的位置（中间）
    int pdfLabelX = beforeButton.getRight() + spacing;
    pdfFileNameLabel.setBounds(pdfLabelX, controlsY, pdfLabelWidth, pdfLabelHeight);

    // 设置 Next 按钮的位置（右侧）
    int nextButtonX = pdfFileNameLabel.getRight() + spacing;
    nextButton.setBounds(nextButtonX, controlsY, buttonWidth, buttonHeight);

    // 调整标签和按钮的高度，使其对齐
    int labelButtonHeight = std::max({ pdfLabelHeight, buttonHeight });
    pdfFileNameLabel.setSize(pdfLabelWidth, labelButtonHeight);
    beforeButton.setSize(buttonWidth, labelButtonHeight);
    nextButton.setSize(buttonWidth, labelButtonHeight);

    // 设置预览框的位置，放在右下角，与主 PDF 框下边缘对齐
    int previewWidth = (availableWidth - pdfWidth - spacing);
    int previewHeight = pdfHeight / 2;
    int previewX = getWidth() - margin - previewWidth;
    int previewY = pdfY + pdfHeight - previewHeight;

    nextPagePreview.setBounds(previewX, previewY, previewWidth, previewHeight);

    // 设置进度条和波形显示的位置（保持对称）
    int audioLabelWidth = 100;  // 音频标签的宽度（左右相同）
    int sliderY = getHeight() - margin - buttonHeight - spacing - sliderHeight - spacing - labelHeight;

    // 首先，保持 waveformDisplay 的宽度和位置不变
    int waveformDisplayWidth = static_cast<int>(availableWidth * 0.7); // 70% 的 availableWidth
    int waveformDisplayX = margin + (availableWidth - waveformDisplayWidth) / 2;
    waveformDisplay.setBounds(waveformDisplayX, sliderY, waveformDisplayWidth, sliderHeight);

    // 然后，设置 progressSlider 的宽度，使其比 waveformDisplay 短一些
    int progressSliderWidth = static_cast<int>(waveformDisplayWidth * 1.03); // 使 progressSlider 的宽度为 waveformDisplay 的 103%
    int progressSliderX = waveformDisplayX + (waveformDisplayWidth - progressSliderWidth) / 2; // 居中对齐到 waveformDisplay
    progressSlider.setBounds(progressSliderX, sliderY, progressSliderWidth, sliderHeight);

    // 设置 markerSlider 的位置，放在 progressSlider 的上方
    int markerSliderHeight = 10; // 可以根据需要调整高度
    int markerSliderY = sliderY - markerSliderHeight - 5; // 与 progressSlider 保持间距
    markerSlider.setBounds(waveformDisplayX, markerSliderY, waveformDisplayWidth, markerSliderHeight);
    // 设置 saveMarkersButton 的位置
    int buttonX = markerSlider.getRight() + spacing; // 在 markerSlider 右侧
    int buttonY = markerSlider.getY() + (markerSlider.getHeight() - buttonHeight) / 2; // 垂直居中
    saveMarkersButton.setBounds(buttonX, buttonY, buttonWidth + 10, buttonHeight);
    // 计算左侧剩余空间，并将 audioPositionLabel 居中
    int leftSpace = progressSliderX - margin;
    int audioPositionLabelX = margin + (leftSpace - audioLabelWidth) / 2;
    audioPositionLabel.setBounds(audioPositionLabelX, sliderY, audioLabelWidth, sliderHeight);
    audioPositionLabel.setJustificationType(juce::Justification::centred); // 设置文字居中

    // 计算右侧剩余空间，并将 audioLengthLabel 居中
    int rightSpace = getWidth() - margin - (progressSliderX + progressSliderWidth);
    int audioLengthLabelX = progressSliderX + progressSliderWidth + (rightSpace - audioLabelWidth) / 2;
    audioLengthLabel.setBounds(audioLengthLabelX, sliderY, audioLabelWidth, sliderHeight);
    audioLengthLabel.setJustificationType(juce::Justification::centred); // 设置文字居中

    // 设置音频文件名标签和播放/暂停按钮的位置，使其水平对齐
    int audioFileNameLabelWidth = 100;  // 调整宽度
    int audioFileNameLabelHeight = buttonHeight;  // 高度与按钮相同
    int audioFileNameLabelX = margin;
    int audioFileNameLabelY = progressSlider.getBottom() + spacing;

    audioFileNameLabel.setBounds(audioFileNameLabelX, audioFileNameLabelY, audioFileNameLabelWidth, audioFileNameLabelHeight);

    // 设置播放和暂停按钮，位于音频文件名标签的右侧
    int buttonsY = audioFileNameLabel.getY();
    playButton.setBounds(audioFileNameLabel.getRight() + spacing, buttonsY, buttonWidth, buttonHeight);
    pauseButton.setBounds(playButton.getRight() + spacing, buttonsY, buttonWidth, buttonHeight);
    
}
