// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBACKINGSTOREDEFAULTCOMPOSITOR_P_H
#define QBACKINGSTOREDEFAULTCOMPOSITOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformbackingstore.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

class QBackingStoreDefaultCompositor
{
public:
    ~QBackingStoreDefaultCompositor();

    void reset();

    QRhiTexture *toTexture(const QPlatformBackingStore *backingStore,
                           QRhi *rhi,
                           QRhiResourceUpdateBatch *resourceUpdates,
                           const QRegion &dirtyRegion,
                           QPlatformBackingStore::TextureFlags *flags) const;

    QPlatformBackingStore::FlushResult flush(QPlatformBackingStore *backingStore,
                                             QRhi *rhi,
                                             QRhiSwapChain *swapchain,
                                             QWindow *window,
                                             qreal sourceDevicePixelRatio,
                                             const QRegion &region,
                                             const QPoint &offset,
                                             QPlatformTextureList *textures,
                                             bool translucentBackground);

private:
    void ensureResources(QRhiSwapChain *swapchain, QRhiResourceUpdateBatch *resourceUpdates);
    QRhiTexture *toTexture(const QImage &image,
                           QRhi *rhi,
                           QRhiResourceUpdateBatch *resourceUpdates,
                           const QRegion &dirtyRegion,
                           QPlatformBackingStore::TextureFlags *flags) const;

    mutable QRhi *m_rhi = nullptr;
    mutable QRhiTexture *m_texture = nullptr;

    QRhiBuffer *m_vbuf = nullptr;
    QRhiSampler *m_sampler = nullptr;
    QRhiGraphicsPipeline *m_psNoBlend = nullptr;
    QRhiGraphicsPipeline *m_psBlend = nullptr;
    QRhiGraphicsPipeline *m_psPremulBlend = nullptr;

    struct PerQuadData {
        QRhiBuffer *ubuf = nullptr;
        // All srbs are layout-compatible.
        QRhiShaderResourceBindings *srb = nullptr;
        QRhiTexture *lastUsedTexture = nullptr;
        bool isValid() const { return ubuf && srb; }
        void reset() {
            delete ubuf;
            ubuf = nullptr;
            delete srb;
            srb = nullptr;
            lastUsedTexture = nullptr;
        }
    };
    PerQuadData m_widgetQuadData;
    QVarLengthArray<PerQuadData, 8> m_textureQuadData;

    PerQuadData createPerQuadData(QRhiTexture *texture);
    void updatePerQuadData(PerQuadData *d, QRhiTexture *texture);
    void updateUniforms(PerQuadData *d, QRhiResourceUpdateBatch *resourceUpdates,
                        const QMatrix4x4 &target, const QMatrix3x3 &source, bool needsRedBlueSwap);
};

QT_END_NAMESPACE

#endif // QBACKINGSTOREDEFAULTCOMPOSITOR_P_H
