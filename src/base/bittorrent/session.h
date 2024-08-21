/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2015-2024  Vladimir Golovnev <glassez@yandex.ru>
 * Copyright (C) 2006  Christophe Dumez <chris@qbittorrent.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#pragma once

#include <QtContainerFwd>
#include <QObject>

#include "base/pathfwd.h"
#include "base/tagset.h"
#include "addtorrentparams.h"
#include "categoryoptions.h"
#include "sharelimitaction.h"
#include "torrentcontentremoveoption.h"
#include "trackerentry.h"
#include "trackerentrystatus.h"

class QString;

namespace BitTorrent
{
    class InfoHash;
    class Torrent;
    class TorrentDescriptor;
    class TorrentID;
    class TorrentInfo;
    struct CacheStatus;
    struct SessionStatus;

    enum class TorrentRemoveOption
    {
        KeepContent,
        RemoveContent
    };

    // Using `Q_ENUM_NS()` without a wrapper namespace in our case is not advised
    // since `Q_NAMESPACE` cannot be used when the same namespace resides at different files.
    // https://www.kdab.com/new-qt-5-8-meta-object-support-namespaces/#comment-143779
    inline namespace SessionSettingsEnums
    {
        Q_NAMESPACE

        enum class BTProtocol : int
        {
            Both = 0,
            TCP = 1,
            UTP = 2
        };
        Q_ENUM_NS(BTProtocol)

        enum class ChokingAlgorithm : int
        {
            FixedSlots = 0,
            RateBased = 1
        };
        Q_ENUM_NS(ChokingAlgorithm)

        enum class DiskIOReadMode : int
        {
            DisableOSCache = 0,
            EnableOSCache = 1
        };
        Q_ENUM_NS(DiskIOReadMode)

        enum class DiskIOType : int
        {
            Default = 0,
            MMap = 1,
            Posix = 2
        };
        Q_ENUM_NS(DiskIOType)

        enum class DiskIOWriteMode : int
        {
            DisableOSCache = 0,
            EnableOSCache = 1,
#ifdef QBT_USES_LIBTORRENT2
            WriteThrough = 2
#endif
        };
        Q_ENUM_NS(DiskIOWriteMode)

        enum class MixedModeAlgorithm : int
        {
            TCP = 0,
            Proportional = 1
        };
        Q_ENUM_NS(MixedModeAlgorithm)

        enum class SeedChokingAlgorithm : int
        {
            RoundRobin = 0,
            FastestUpload = 1,
            AntiLeech = 2
        };
        Q_ENUM_NS(SeedChokingAlgorithm)

        enum class ResumeDataStorageType
        {
            Legacy,
            SQLite
        };
        Q_ENUM_NS(ResumeDataStorageType)
    }

    class Session : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Session)

    public:
        static void initInstance();
        static void freeInstance();
        static Session *instance();

        using QObject::QObject;

        virtual Path savePath() const = 0;
        virtual void setSavePath(const Path &path) = 0;
        virtual Path downloadPath() const = 0;
        virtual void setDownloadPath(const Path &path) = 0;
        virtual bool isDownloadPathEnabled() const = 0;
        virtual void setDownloadPathEnabled(bool enabled) = 0;

        static bool isValidCategoryName(const QString &name);
        static QString subcategoryName(const QString &category);
        static QString parentCategoryName(const QString &category);
        // returns category itself and all top level categories
        static QStringList expandCategory(const QString &category);

        virtual QStringList categories() const = 0;
        virtual CategoryOptions categoryOptions(const QString &categoryName) const = 0;
        virtual Path categorySavePath(const QString &categoryName) const = 0;
        virtual Path categorySavePath(const QString &categoryName, const CategoryOptions &options) const = 0;
        virtual Path categoryDownloadPath(const QString &categoryName) const = 0;
        virtual Path categoryDownloadPath(const QString &categoryName, const CategoryOptions &options) const = 0;
        virtual bool addCategory(const QString &name, const CategoryOptions &options = {}) = 0;
        virtual bool editCategory(const QString &name, const CategoryOptions &options) = 0;
        virtual bool removeCategory(const QString &name) = 0;
        virtual bool isSubcategoriesEnabled() const = 0;
        virtual void setSubcategoriesEnabled(bool value) = 0;
        virtual bool useCategoryPathsInManualMode() const = 0;
        virtual void setUseCategoryPathsInManualMode(bool value) = 0;

        virtual Path suggestedSavePath(const QString &categoryName, std::optional<bool> useAutoTMM) const = 0;
        virtual Path suggestedDownloadPath(const QString &categoryName, std::optional<bool> useAutoTMM) const = 0;

        virtual TagSet tags() const = 0;
        virtual bool hasTag(const Tag &tag) const = 0;
        virtual bool addTag(const Tag &tag) = 0;
        virtual bool removeTag(const Tag &tag) = 0;

        // Torrent Management Mode subsystem (TMM)
        //
        // Each torrent can be either in Manual mode or in Automatic mode
        // In Manual Mode various torrent properties are set explicitly(eg save path)
        // In Automatic Mode various torrent properties are set implicitly(eg save path)
        //     based on the associated category.
        // In Automatic Mode torrent save path can be changed in following cases:
        //     1. Default save path changed
        //     2. Torrent category save path changed
        //     3. Torrent category changed
        //     (unless otherwise is specified)
        virtual bool isAutoTMMDisabledByDefault() const = 0;
        virtual void setAutoTMMDisabledByDefault(bool value) = 0;
        virtual bool isDisableAutoTMMWhenCategoryChanged() const = 0;
        virtual void setDisableAutoTMMWhenCategoryChanged(bool value) = 0;
        virtual bool isDisableAutoTMMWhenDefaultSavePathChanged() const = 0;
        virtual void setDisableAutoTMMWhenDefaultSavePathChanged(bool value) = 0;
        virtual bool isDisableAutoTMMWhenCategorySavePathChanged() const = 0;
        virtual void setDisableAutoTMMWhenCategorySavePathChanged(bool value) = 0;

        virtual qreal globalMaxRatio() const = 0;
        virtual void setGlobalMaxRatio(qreal ratio) = 0;
        virtual int globalMaxSeedingMinutes() const = 0;
        virtual void setGlobalMaxSeedingMinutes(int minutes) = 0;
        virtual int globalMaxInactiveSeedingMinutes() const = 0;
        virtual void setGlobalMaxInactiveSeedingMinutes(int minutes) = 0;
        virtual ShareLimitAction shareLimitAction() const = 0;
        virtual void setShareLimitAction(ShareLimitAction act) = 0;

        virtual QString getDHTBootstrapNodes() const = 0;
        virtual void setDHTBootstrapNodes(const QString &nodes) = 0;
        virtual bool isDHTEnabled() const = 0;
        virtual void setDHTEnabled(bool enabled) = 0;
        virtual bool isLSDEnabled() const = 0;
        virtual void setLSDEnabled(bool enabled) = 0;
        virtual bool isPeXEnabled() const = 0;
        virtual void setPeXEnabled(bool enabled) = 0;
        virtual bool isAddTorrentToQueueTop() const = 0;
        virtual void setAddTorrentToQueueTop(bool value) = 0;
        virtual bool isAddTorrentStopped() const = 0;
        virtual void setAddTorrentStopped(bool value) = 0;
        virtual Torrent::StopCondition torrentStopCondition() const = 0;
        virtual void setTorrentStopCondition(Torrent::StopCondition stopCondition) = 0;
        virtual TorrentContentLayout torrentContentLayout() const = 0;
        virtual void setTorrentContentLayout(TorrentContentLayout value) = 0;
        virtual bool isTrackerEnabled() const = 0;
        virtual void setTrackerEnabled(bool enabled) = 0;
        virtual bool isAppendExtensionEnabled() const = 0;
        virtual void setAppendExtensionEnabled(bool enabled) = 0;
        virtual bool isUnwantedFolderEnabled() const = 0;
        virtual void setUnwantedFolderEnabled(bool enabled) = 0;
        virtual int refreshInterval() const = 0;
        virtual void setRefreshInterval(int value) = 0;
        virtual bool isPreallocationEnabled() const = 0;
        virtual void setPreallocationEnabled(bool enabled) = 0;
        virtual Path torrentExportDirectory() const = 0;
        virtual void setTorrentExportDirectory(const Path &path) = 0;
        virtual Path finishedTorrentExportDirectory() const = 0;
        virtual void setFinishedTorrentExportDirectory(const Path &path) = 0;

        bool isPerformanceWarningEnabled() const;
        void setPerformanceWarningEnabled(bool enable);
        int saveResumeDataInterval() const;
        void setSaveResumeDataInterval(int value);
        QMap<QString, QVariant> ports() const;
        void setPorts(const QMap<QString, QVariant> ports);
        QMap<QString, QVariant> portsEnabled() const;
        void setPortsEnabled(const QMap<QString, QVariant> portsEnabled);
        QString networkInterface() const;
        void setNetworkInterface(const QString &iface);
        QString networkInterfaceName() const;
        void setNetworkInterfaceName(const QString &name);
        QString networkInterfaceAddress() const;
        void setNetworkInterfaceAddress(const QString &address);
        int encryption() const;
        void setEncryption(int state);
        int maxActiveCheckingTorrents() const;
        void setMaxActiveCheckingTorrents(int val);
        bool isProxyPeerConnectionsEnabled() const;
        void setProxyPeerConnectionsEnabled(bool enabled);
        ChokingAlgorithm chokingAlgorithm() const;
        void setChokingAlgorithm(ChokingAlgorithm mode);
        SeedChokingAlgorithm seedChokingAlgorithm() const;
        void setSeedChokingAlgorithm(SeedChokingAlgorithm mode);
        bool isAddTrackersEnabled() const;
        void setAddTrackersEnabled(bool enabled);
        QString additionalTrackers() const;
        void setAdditionalTrackers(const QString &trackers);
        bool isIPFilteringEnabled() const;
        void setIPFilteringEnabled(bool enabled);
        Path IPFilterFile() const;
        void setIPFilterFile(const Path &path);
        bool announceToAllTrackers() const;
        void setAnnounceToAllTrackers(bool val);
        bool announceToAllTiers() const;
        void setAnnounceToAllTiers(bool val);
        int peerTurnover() const;
        void setPeerTurnover(int val);
        int peerTurnoverCutoff() const;
        void setPeerTurnoverCutoff(int val);
        int peerTurnoverInterval() const;
        void setPeerTurnoverInterval(int val);
        int requestQueueSize() const;
        void setRequestQueueSize(int val);
        int asyncIOThreads() const;
        void setAsyncIOThreads(int num);
        int hashingThreads() const;
        void setHashingThreads(int num);
        int filePoolSize() const;
        void setFilePoolSize(int size);
        int checkingMemUsage() const;
        void setCheckingMemUsage(int size);
        int diskCacheSize() const;
        void setDiskCacheSize(int size);
        int diskCacheTTL() const;
        void setDiskCacheTTL(int ttl);
        qint64 diskQueueSize() const;
        void setDiskQueueSize(qint64 size);
        DiskIOType diskIOType() const;
        void setDiskIOType(DiskIOType type);
        DiskIOReadMode diskIOReadMode() const;
        void setDiskIOReadMode(DiskIOReadMode mode);
        DiskIOWriteMode diskIOWriteMode() const;
        void setDiskIOWriteMode(DiskIOWriteMode mode);
        bool isCoalesceReadWriteEnabled() const;
        void setCoalesceReadWriteEnabled(bool enabled);
        bool usePieceExtentAffinity() const;
        void setPieceExtentAffinity(bool enabled);
        bool isSuggestModeEnabled() const;
        void setSuggestMode(bool mode);
        int sendBufferWatermark() const;
        void setSendBufferWatermark(int value);
        int sendBufferLowWatermark() const;
        void setSendBufferLowWatermark(int value);
        int sendBufferWatermarkFactor() const;
        void setSendBufferWatermarkFactor(int value);
        int connectionSpeed() const;
        void setConnectionSpeed(int value);
        int socketBacklogSize() const;
        void setSocketBacklogSize(int value);
        bool isAnonymousModeEnabled() const;
        void setAnonymousModeEnabled(bool enabled);
        bool isQueueingSystemEnabled() const;
        void setQueueingSystemEnabled(bool enabled);
        bool ignoreSlowTorrentsForQueueing() const;
        void setIgnoreSlowTorrentsForQueueing(bool ignore);
        int downloadRateForSlowTorrents() const;
        void setDownloadRateForSlowTorrents(int rateInKibiBytes);
        int uploadRateForSlowTorrents() const;
        void setUploadRateForSlowTorrents(int rateInKibiBytes);
        int slowTorrentsInactivityTimer() const;
        void setSlowTorrentsInactivityTimer(int timeInSeconds);
        int outgoingPortsMin() const;
        void setOutgoingPortsMin(int min);
        int outgoingPortsMax() const;
        void setOutgoingPortsMax(int max);
        int UPnPLeaseDuration() const;
        void setUPnPLeaseDuration(int duration);
        int peerToS() const;
        void setPeerToS(int value);
        bool ignoreLimitsOnLAN() const;
        void setIgnoreLimitsOnLAN(bool ignore);
        bool includeOverheadInLimits() const;
        void setIncludeOverheadInLimits(bool include);
        QString announceIP() const;
        void setAnnounceIP(const QString &ip);
        int maxConcurrentHTTPAnnounces() const;
        void setMaxConcurrentHTTPAnnounces(int value);
        bool isReannounceWhenAddressChangedEnabled() const;
        void setReannounceWhenAddressChangedEnabled(bool enabled);
        void reannounceToAllTrackers() const;
        int stopTrackerTimeout() const;
        void setStopTrackerTimeout(int value);
        int maxConnections() const;
        void setMaxConnections(int max);
        int maxConnectionsPerTorrent() const;
        void setMaxConnectionsPerTorrent(int max);
        int maxUploads() const;
        void setMaxUploads(int max);
        int maxUploadsPerTorrent() const;
        void setMaxUploadsPerTorrent(int max);
        int maxActiveDownloads() const;
        void setMaxActiveDownloads(int max);
        int maxActiveUploads() const;
        void setMaxActiveUploads(int max);
        int maxActiveTorrents() const;
        void setMaxActiveTorrents(int max);
        BTProtocol btProtocol() const;
        void setBTProtocol(BTProtocol protocol);
        bool isUTPRateLimited() const;
        void setUTPRateLimited(bool limited);
        MixedModeAlgorithm utpMixedMode() const;
        void setUtpMixedMode(MixedModeAlgorithm mode);
        bool isIDNSupportEnabled() const;
        void setIDNSupportEnabled(bool enabled);
        bool multiConnectionsPerIpEnabled() const;
        void setMultiConnectionsPerIpEnabled(bool enabled);
        bool validateHTTPSTrackerCertificate() const;
        void setValidateHTTPSTrackerCertificate(bool enabled);
        bool isSSRFMitigationEnabled() const;
        void setSSRFMitigationEnabled(bool enabled);
        bool blockPeersOnPrivilegedPorts() const;
        void setBlockPeersOnPrivilegedPorts(bool enabled);
        bool isTrackerFilteringEnabled() const;
        void setTrackerFilteringEnabled(bool enabled);
        bool isExcludedFileNamesEnabled() const;
        void setExcludedFileNamesEnabled(const bool enabled);
        QStringList excludedFileNames() const;
        void setExcludedFileNames(const QStringList &newList);
        bool isFilenameExcluded(const QString &fileName) const;
        QStringList bannedIPs() const;
        void setBannedIPs(const QStringList &newList);
        ResumeDataStorageType resumeDataStorageType() const;
        void setResumeDataStorageType(ResumeDataStorageType type);

        virtual bool isPerformanceWarningEnabled() const = 0;
        virtual void setPerformanceWarningEnabled(bool enable) = 0;
        virtual int saveResumeDataInterval() const = 0;
        virtual void setSaveResumeDataInterval(int value) = 0;
        virtual int shutdownTimeout() const = 0;
        virtual void setShutdownTimeout(int value) = 0;
        virtual int port() const = 0;
        virtual void setPort(int port) = 0;
        virtual bool isSSLEnabled() const = 0;
        virtual void setSSLEnabled(bool enabled) = 0;
        virtual int sslPort() const = 0;
        virtual void setSSLPort(int port) = 0;
        virtual QString networkInterface() const = 0;
        virtual void setNetworkInterface(const QString &iface) = 0;
        virtual QString networkInterfaceName() const = 0;
        virtual void setNetworkInterfaceName(const QString &name) = 0;
        virtual QString networkInterfaceAddress() const = 0;
        virtual void setNetworkInterfaceAddress(const QString &address) = 0;
        virtual int encryption() const = 0;
        virtual void setEncryption(int state) = 0;
        virtual int maxActiveCheckingTorrents() const = 0;
        virtual void setMaxActiveCheckingTorrents(int val) = 0;
        virtual bool isI2PEnabled() const = 0;
        virtual void setI2PEnabled(bool enabled) = 0;
        virtual QString I2PAddress() const = 0;
        virtual void setI2PAddress(const QString &address) = 0;
        virtual int I2PPort() const = 0;
        virtual void setI2PPort(int port) = 0;
        virtual bool I2PMixedMode() const = 0;
        virtual void setI2PMixedMode(bool enabled) = 0;
        virtual int I2PInboundQuantity() const = 0;
        virtual void setI2PInboundQuantity(int value) = 0;
        virtual int I2POutboundQuantity() const = 0;
        virtual void setI2POutboundQuantity(int value) = 0;
        virtual int I2PInboundLength() const = 0;
        virtual void setI2PInboundLength(int value) = 0;
        virtual int I2POutboundLength() const = 0;
        virtual void setI2POutboundLength(int value) = 0;
        virtual bool isProxyPeerConnectionsEnabled() const = 0;
        virtual void setProxyPeerConnectionsEnabled(bool enabled) = 0;
        virtual ChokingAlgorithm chokingAlgorithm() const = 0;
        virtual void setChokingAlgorithm(ChokingAlgorithm mode) = 0;
        virtual SeedChokingAlgorithm seedChokingAlgorithm() const = 0;
        virtual void setSeedChokingAlgorithm(SeedChokingAlgorithm mode) = 0;
        virtual bool isAddTrackersEnabled() const = 0;
        virtual void setAddTrackersEnabled(bool enabled) = 0;
        virtual QString additionalTrackers() const = 0;
        virtual void setAdditionalTrackers(const QString &trackers) = 0;
        virtual bool isIPFilteringEnabled() const = 0;
        virtual void setIPFilteringEnabled(bool enabled) = 0;
        virtual Path IPFilterFile() const = 0;
        virtual void setIPFilterFile(const Path &path) = 0;
        virtual bool announceToAllTrackers() const = 0;
        virtual void setAnnounceToAllTrackers(bool val) = 0;
        virtual bool announceToAllTiers() const = 0;
        virtual void setAnnounceToAllTiers(bool val) = 0;
        virtual int peerTurnover() const = 0;
        virtual void setPeerTurnover(int val) = 0;
        virtual int peerTurnoverCutoff() const = 0;
        virtual void setPeerTurnoverCutoff(int val) = 0;
        virtual int peerTurnoverInterval() const = 0;
        virtual void setPeerTurnoverInterval(int val) = 0;
        virtual int requestQueueSize() const = 0;
        virtual void setRequestQueueSize(int val) = 0;
        virtual int asyncIOThreads() const = 0;
        virtual void setAsyncIOThreads(int num) = 0;
        virtual int hashingThreads() const = 0;
        virtual void setHashingThreads(int num) = 0;
        virtual int filePoolSize() const = 0;
        virtual void setFilePoolSize(int size) = 0;
        virtual int checkingMemUsage() const = 0;
        virtual void setCheckingMemUsage(int size) = 0;
        virtual int diskCacheSize() const = 0;
        virtual void setDiskCacheSize(int size) = 0;
        virtual int diskCacheTTL() const = 0;
        virtual void setDiskCacheTTL(int ttl) = 0;
        virtual qint64 diskQueueSize() const = 0;
        virtual void setDiskQueueSize(qint64 size) = 0;
        virtual DiskIOType diskIOType() const = 0;
        virtual void setDiskIOType(DiskIOType type) = 0;
        virtual DiskIOReadMode diskIOReadMode() const = 0;
        virtual void setDiskIOReadMode(DiskIOReadMode mode) = 0;
        virtual DiskIOWriteMode diskIOWriteMode() const = 0;
        virtual void setDiskIOWriteMode(DiskIOWriteMode mode) = 0;
        virtual bool isCoalesceReadWriteEnabled() const = 0;
        virtual void setCoalesceReadWriteEnabled(bool enabled) = 0;
        virtual bool usePieceExtentAffinity() const = 0;
        virtual void setPieceExtentAffinity(bool enabled) = 0;
        virtual bool isSuggestModeEnabled() const = 0;
        virtual void setSuggestMode(bool mode) = 0;
        virtual int sendBufferWatermark() const = 0;
        virtual void setSendBufferWatermark(int value) = 0;
        virtual int sendBufferLowWatermark() const = 0;
        virtual void setSendBufferLowWatermark(int value) = 0;
        virtual int sendBufferWatermarkFactor() const = 0;
        virtual void setSendBufferWatermarkFactor(int value) = 0;
        virtual int connectionSpeed() const = 0;
        virtual void setConnectionSpeed(int value) = 0;
        virtual int socketSendBufferSize() const = 0;
        virtual void setSocketSendBufferSize(int value) = 0;
        virtual int socketReceiveBufferSize() const = 0;
        virtual void setSocketReceiveBufferSize(int value) = 0;
        virtual int socketBacklogSize() const = 0;
        virtual void setSocketBacklogSize(int value) = 0;
        virtual bool isAnonymousModeEnabled() const = 0;
        virtual void setAnonymousModeEnabled(bool enabled) = 0;
        virtual bool isQueueingSystemEnabled() const = 0;
        virtual void setQueueingSystemEnabled(bool enabled) = 0;
        virtual bool ignoreSlowTorrentsForQueueing() const = 0;
        virtual void setIgnoreSlowTorrentsForQueueing(bool ignore) = 0;
        virtual int downloadRateForSlowTorrents() const = 0;
        virtual void setDownloadRateForSlowTorrents(int rateInKibiBytes) = 0;
        virtual int uploadRateForSlowTorrents() const = 0;
        virtual void setUploadRateForSlowTorrents(int rateInKibiBytes) = 0;
        virtual int slowTorrentsInactivityTimer() const = 0;
        virtual void setSlowTorrentsInactivityTimer(int timeInSeconds) = 0;
        virtual int outgoingPortsMin() const = 0;
        virtual void setOutgoingPortsMin(int min) = 0;
        virtual int outgoingPortsMax() const = 0;
        virtual void setOutgoingPortsMax(int max) = 0;
        virtual int UPnPLeaseDuration() const = 0;
        virtual void setUPnPLeaseDuration(int duration) = 0;
        virtual int peerToS() const = 0;
        virtual void setPeerToS(int value) = 0;
        virtual bool ignoreLimitsOnLAN() const = 0;
        virtual void setIgnoreLimitsOnLAN(bool ignore) = 0;
        virtual bool includeOverheadInLimits() const = 0;
        virtual void setIncludeOverheadInLimits(bool include) = 0;
        virtual QString announceIP() const = 0;
        virtual void setAnnounceIP(const QString &ip) = 0;
        virtual int maxConcurrentHTTPAnnounces() const = 0;
        virtual void setMaxConcurrentHTTPAnnounces(int value) = 0;
        virtual bool isReannounceWhenAddressChangedEnabled() const = 0;
        virtual void setReannounceWhenAddressChangedEnabled(bool enabled) = 0;
        virtual void reannounceToAllTrackers() const = 0;
        virtual int stopTrackerTimeout() const = 0;
        virtual void setStopTrackerTimeout(int value) = 0;
        virtual int maxConnections() const = 0;
        virtual void setMaxConnections(int max) = 0;
        virtual int maxConnectionsPerTorrent() const = 0;
        virtual void setMaxConnectionsPerTorrent(int max) = 0;
        virtual int maxUploads() const = 0;
        virtual void setMaxUploads(int max) = 0;
        virtual int maxUploadsPerTorrent() const = 0;
        virtual void setMaxUploadsPerTorrent(int max) = 0;
        virtual int maxActiveDownloads() const = 0;
        virtual void setMaxActiveDownloads(int max) = 0;
        virtual int maxActiveUploads() const = 0;
        virtual void setMaxActiveUploads(int max) = 0;
        virtual int maxActiveTorrents() const = 0;
        virtual void setMaxActiveTorrents(int max) = 0;
        virtual BTProtocol btProtocol() const = 0;
        virtual void setBTProtocol(BTProtocol protocol) = 0;
        virtual bool isUTPRateLimited() const = 0;
        virtual void setUTPRateLimited(bool limited) = 0;
        virtual MixedModeAlgorithm utpMixedMode() const = 0;
        virtual void setUtpMixedMode(MixedModeAlgorithm mode) = 0;
        virtual bool isIDNSupportEnabled() const = 0;
        virtual void setIDNSupportEnabled(bool enabled) = 0;
        virtual bool multiConnectionsPerIpEnabled() const = 0;
        virtual void setMultiConnectionsPerIpEnabled(bool enabled) = 0;
        virtual bool validateHTTPSTrackerCertificate() const = 0;
        virtual void setValidateHTTPSTrackerCertificate(bool enabled) = 0;
        virtual bool isSSRFMitigationEnabled() const = 0;
        virtual void setSSRFMitigationEnabled(bool enabled) = 0;
        virtual bool blockPeersOnPrivilegedPorts() const = 0;
        virtual void setBlockPeersOnPrivilegedPorts(bool enabled) = 0;
        virtual bool isTrackerFilteringEnabled() const = 0;
        virtual void setTrackerFilteringEnabled(bool enabled) = 0;
        virtual bool isExcludedFileNamesEnabled() const = 0;
        virtual void setExcludedFileNamesEnabled(bool enabled) = 0;
        virtual QStringList excludedFileNames() const = 0;
        virtual void setExcludedFileNames(const QStringList &newList) = 0;
        virtual void applyFilenameFilter(const PathList &files, QList<BitTorrent::DownloadPriority> &priorities) = 0;
        virtual QStringList bannedIPs() const = 0;
        virtual void setBannedIPs(const QStringList &newList) = 0;
        virtual ResumeDataStorageType resumeDataStorageType() const = 0;
        virtual void setResumeDataStorageType(ResumeDataStorageType type) = 0;
        virtual bool isMergeTrackersEnabled() const = 0;
        virtual void setMergeTrackersEnabled(bool enabled) = 0;
        virtual bool isStartPaused() const = 0;
        virtual void setStartPaused(bool value) = 0;
        virtual TorrentContentRemoveOption torrentContentRemoveOption() const = 0;
        virtual void setTorrentContentRemoveOption(TorrentContentRemoveOption option) = 0;

        virtual bool isRestored() const = 0;

        virtual bool isPaused() const = 0;
        virtual void pause() = 0;
        virtual void resume() = 0;

        virtual Torrent *getTorrent(const TorrentID &id) const = 0;
        virtual Torrent *findTorrent(const InfoHash &infoHash) const = 0;
        virtual QList<Torrent *> torrents() const = 0;
        virtual qsizetype torrentsCount() const = 0;
        virtual const SessionStatus &status() const = 0;
        virtual const CacheStatus &cacheStatus() const = 0;
        virtual bool isListening() const = 0;

        virtual void banIP(const QString &ip) = 0;

        virtual bool isKnownTorrent(const InfoHash &infoHash) const = 0;
        virtual bool addTorrent(const TorrentDescriptor &torrentDescr, const AddTorrentParams &params = {}) = 0;
        virtual bool removeTorrent(const TorrentID &id, TorrentRemoveOption deleteOption = TorrentRemoveOption::KeepContent) = 0;
        virtual bool downloadMetadata(const TorrentDescriptor &torrentDescr) = 0;
        virtual bool cancelDownloadMetadata(const TorrentID &id) = 0;

        virtual void increaseTorrentsQueuePos(const QList<TorrentID> &ids) = 0;
        virtual void decreaseTorrentsQueuePos(const QList<TorrentID> &ids) = 0;
        virtual void topTorrentsQueuePos(const QList<TorrentID> &ids) = 0;
        virtual void bottomTorrentsQueuePos(const QList<TorrentID> &ids) = 0;

        QStringList getNetworkInterfaces() const;

    signals:
        void startupProgressUpdated(int progress);
        void addTorrentFailed(const InfoHash &infoHash, const QString &reason);
        void allTorrentsFinished();
        void categoryAdded(const QString &categoryName);
        void categoryRemoved(const QString &categoryName);
        void categoryOptionsChanged(const QString &categoryName);
        void fullDiskError(Torrent *torrent, const QString &msg);
        void IPFilterParsed(bool error, int ruleCount);
        void loadTorrentFailed(const QString &error);
        void metadataDownloaded(const TorrentInfo &info);
        void restored();
        void paused();
        void resumed();
        void speedLimitModeChanged(bool alternative);
        void statsUpdated();
        void subcategoriesSupportChanged();
        void tagAdded(const Tag &tag);
        void tagRemoved(const Tag &tag);
        void torrentAboutToBeRemoved(Torrent *torrent);
        void torrentAdded(Torrent *torrent);
        void torrentCategoryChanged(Torrent *torrent, const QString &oldCategory);
        void torrentFinished(Torrent *torrent);
        void torrentFinishedChecking(Torrent *torrent);
        void torrentMetadataReceived(Torrent *torrent);
        void torrentStopped(Torrent *torrent);
        void torrentStarted(Torrent *torrent);
        void torrentSavePathChanged(Torrent *torrent);
        void torrentSavingModeChanged(Torrent *torrent);
        void torrentsLoaded(const QList<Torrent *> &torrents);
        void torrentsUpdated(const QList<Torrent *> &torrents);
        void torrentTagAdded(Torrent *torrent, const Tag &tag);
        void torrentTagRemoved(Torrent *torrent, const Tag &tag);
        void trackerError(Torrent *torrent, const QString &tracker);
        void trackersAdded(Torrent *torrent, const QList<TrackerEntry> &trackers);
        void trackersChanged(Torrent *torrent);
        void trackersRemoved(Torrent *torrent, const QStringList &trackers);
        void trackerSuccess(Torrent *torrent, const QString &tracker);
        void trackerWarning(Torrent *torrent, const QString &tracker);
        void trackerEntriesUpdated(const QHash<Torrent *, QSet<QString>> &updateInfos);

    private slots:
        void configureDeferred();
        void readAlerts();
        void enqueueRefresh();
        void processShareLimits();
        void generateResumeData();
        void handleIPFilterParsed(int ruleCount);
        void handleIPFilterError();
        void handleDownloadFinished(const Net::DownloadResult &result);
        void fileSearchFinished(const TorrentID &id, const Path &savePath, const PathList &fileNames);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        // Session reconfiguration triggers
        void networkOnlineStateChanged(bool online);
        void networkConfigurationChange(const QNetworkConfiguration &);
#endif

    private:
        struct ResumeSessionContext;

        struct MoveStorageJob
        {
            lt::torrent_handle torrentHandle;
            Path path;
            MoveStorageMode mode;
        };

        struct RemovingTorrentData
        {
            QString name;
            Path pathToRemove;
            DeleteOption deleteOption;
        };

        explicit Session(QObject *parent = nullptr);
        ~Session();

        bool hasPerTorrentRatioLimit() const;
        bool hasPerTorrentSeedingTimeLimit() const;

        // Session configuration
        Q_INVOKABLE void configure();
        void configureComponents();
        void initializeNativeSession();
        void loadLTSettings(lt::settings_pack &settingsPack);
        void configureNetworkInterfaces(lt::settings_pack &settingsPack);
        void configurePeerClasses();
        void adjustLimits(lt::settings_pack &settingsPack) const;
        void applyBandwidthLimits(lt::settings_pack &settingsPack) const;
        void initMetrics();
        void adjustLimits();
        void applyBandwidthLimits();
        void processBannedIPs(lt::ip_filter &filter);
        QStringList getListeningIPs() const;
        void configureListeningInterface();
        void enableTracker(bool enable);
        void enableBandwidthScheduler();
        void populateAdditionalTrackers();
        void enableIPFilter();
        void disableIPFilter();
        void processTrackerStatuses();
        void populateExcludedFileNamesRegExpList();
        void prepareStartup();
        void handleLoadedResumeData(ResumeSessionContext *context);
        void processNextResumeData(ResumeSessionContext *context);
        void endStartup(ResumeSessionContext *context);

        LoadTorrentParams initLoadTorrentParams(const AddTorrentParams &addTorrentParams);
        bool addTorrent_impl(const std::variant<MagnetUri, TorrentInfo> &source, const AddTorrentParams &addTorrentParams);

        void updateSeedingLimitTimer();
        void exportTorrentFile(const Torrent *torrent, const Path &folderPath);

        void handleAlert(const lt::alert *a);
        void handleAddTorrentAlerts(const std::vector<lt::alert *> &alerts);
        void dispatchTorrentAlert(const lt::alert *a);
        void handleStateUpdateAlert(const lt::state_update_alert *p);
        void handleMetadataReceivedAlert(const lt::metadata_received_alert *p);
        void handleFileErrorAlert(const lt::file_error_alert *p);
        void handleTorrentRemovedAlert(const lt::torrent_removed_alert *p);
        void handleTorrentDeletedAlert(const lt::torrent_deleted_alert *p);
        void handleTorrentDeleteFailedAlert(const lt::torrent_delete_failed_alert *p);
        void handlePortmapWarningAlert(const lt::portmap_error_alert *p);
        void handlePortmapAlert(const lt::portmap_alert *p);
        void handlePeerBlockedAlert(const lt::peer_blocked_alert *p);
        void handlePeerBanAlert(const lt::peer_ban_alert *p);
        void handleUrlSeedAlert(const lt::url_seed_alert *p);
        void handleListenSucceededAlert(const lt::listen_succeeded_alert *p);
        void handleListenFailedAlert(const lt::listen_failed_alert *p);
        void handleExternalIPAlert(const lt::external_ip_alert *p);
        void handleSessionStatsAlert(const lt::session_stats_alert *p);
        void handleAlertsDroppedAlert(const lt::alerts_dropped_alert *p) const;
        void handleStorageMovedAlert(const lt::storage_moved_alert *p);
        void handleStorageMovedFailedAlert(const lt::storage_moved_failed_alert *p);
        void handleSocks5Alert(const lt::socks5_alert *p) const;
        void handleTrackerAlert(const lt::tracker_alert *a);

        TorrentImpl *createTorrent(const lt::torrent_handle &nativeHandle, const LoadTorrentParams &params);

        void saveResumeData();
        void saveTorrentsQueue() const;
        void removeTorrentsQueue() const;

        std::vector<lt::alert *> getPendingAlerts(lt::time_duration time = lt::time_duration::zero()) const;

        void moveTorrentStorage(const MoveStorageJob &job) const;
        void handleMoveTorrentStorageJobFinished(const Path &newPath);

        void loadCategories();
        void storeCategories() const;
        void upgradeCategories();

        // BitTorrent
        lt::session *m_nativeSession = nullptr;

        bool m_deferredConfigureScheduled = false;
        bool m_IPFilteringConfigured = false;
        bool m_listenInterfaceConfigured = false;

        CachedSettingValue<bool> m_isDHTEnabled;
        CachedSettingValue<bool> m_isLSDEnabled;
        CachedSettingValue<bool> m_isPeXEnabled;
        CachedSettingValue<bool> m_isIPFilteringEnabled;
        CachedSettingValue<bool> m_isTrackerFilteringEnabled;
        CachedSettingValue<Path> m_IPFilterFile;
        CachedSettingValue<bool> m_announceToAllTrackers;
        CachedSettingValue<bool> m_announceToAllTiers;
        CachedSettingValue<int> m_asyncIOThreads;
        CachedSettingValue<int> m_hashingThreads;
        CachedSettingValue<int> m_filePoolSize;
        CachedSettingValue<int> m_checkingMemUsage;
        CachedSettingValue<int> m_diskCacheSize;
        CachedSettingValue<int> m_diskCacheTTL;
        CachedSettingValue<qint64> m_diskQueueSize;
        CachedSettingValue<DiskIOType> m_diskIOType;
        CachedSettingValue<DiskIOReadMode> m_diskIOReadMode;
        CachedSettingValue<DiskIOWriteMode> m_diskIOWriteMode;
        CachedSettingValue<bool> m_coalesceReadWriteEnabled;
        CachedSettingValue<bool> m_usePieceExtentAffinity;
        CachedSettingValue<bool> m_isSuggestMode;
        CachedSettingValue<int> m_sendBufferWatermark;
        CachedSettingValue<int> m_sendBufferLowWatermark;
        CachedSettingValue<int> m_sendBufferWatermarkFactor;
        CachedSettingValue<int> m_connectionSpeed;
        CachedSettingValue<int> m_socketBacklogSize;
        CachedSettingValue<bool> m_isAnonymousModeEnabled;
        CachedSettingValue<bool> m_isQueueingEnabled;
        CachedSettingValue<int> m_maxActiveDownloads;
        CachedSettingValue<int> m_maxActiveUploads;
        CachedSettingValue<int> m_maxActiveTorrents;
        CachedSettingValue<bool> m_ignoreSlowTorrentsForQueueing;
        CachedSettingValue<int> m_downloadRateForSlowTorrents;
        CachedSettingValue<int> m_uploadRateForSlowTorrents;
        CachedSettingValue<int> m_slowTorrentsInactivityTimer;
        CachedSettingValue<int> m_outgoingPortsMin;
        CachedSettingValue<int> m_outgoingPortsMax;
        CachedSettingValue<int> m_UPnPLeaseDuration;
        CachedSettingValue<int> m_peerToS;
        CachedSettingValue<bool> m_ignoreLimitsOnLAN;
        CachedSettingValue<bool> m_includeOverheadInLimits;
        CachedSettingValue<QString> m_announceIP;
        CachedSettingValue<int> m_maxConcurrentHTTPAnnounces;
        CachedSettingValue<bool> m_isReannounceWhenAddressChangedEnabled;
        CachedSettingValue<int> m_stopTrackerTimeout;
        CachedSettingValue<int> m_maxConnections;
        CachedSettingValue<int> m_maxUploads;
        CachedSettingValue<int> m_maxConnectionsPerTorrent;
        CachedSettingValue<int> m_maxUploadsPerTorrent;
        CachedSettingValue<BTProtocol> m_btProtocol;
        CachedSettingValue<bool> m_isUTPRateLimited;
        CachedSettingValue<MixedModeAlgorithm> m_utpMixedMode;
        CachedSettingValue<bool> m_IDNSupportEnabled;
        CachedSettingValue<bool> m_multiConnectionsPerIpEnabled;
        CachedSettingValue<bool> m_validateHTTPSTrackerCertificate;
        CachedSettingValue<bool> m_SSRFMitigationEnabled;
        CachedSettingValue<bool> m_blockPeersOnPrivilegedPorts;
        CachedSettingValue<bool> m_isAddTrackersEnabled;
        CachedSettingValue<QString> m_additionalTrackers;
        CachedSettingValue<qreal> m_globalMaxRatio;
        CachedSettingValue<int> m_globalMaxSeedingMinutes;
        CachedSettingValue<bool> m_isAddTorrentPaused;
        CachedSettingValue<TorrentContentLayout> m_torrentContentLayout;
        CachedSettingValue<bool> m_isAppendExtensionEnabled;
        CachedSettingValue<int> m_refreshInterval;
        CachedSettingValue<bool> m_isPreallocationEnabled;
        CachedSettingValue<Path> m_torrentExportDirectory;
        CachedSettingValue<Path> m_finishedTorrentExportDirectory;
        CachedSettingValue<int> m_globalDownloadSpeedLimit;
        CachedSettingValue<int> m_globalUploadSpeedLimit;
        CachedSettingValue<int> m_altGlobalDownloadSpeedLimit;
        CachedSettingValue<int> m_altGlobalUploadSpeedLimit;
        CachedSettingValue<bool> m_isAltGlobalSpeedLimitEnabled;
        CachedSettingValue<bool> m_isBandwidthSchedulerEnabled;
        CachedSettingValue<bool> m_isPerformanceWarningEnabled;
        CachedSettingValue<int> m_saveResumeDataInterval;
        CachedSettingValue<QMap<QString, QVariant>> m_ports;
        CachedSettingValue<QMap<QString, QVariant>> m_portsEnabled;
        CachedSettingValue<QString> m_networkInterface;
        CachedSettingValue<QString> m_networkInterfaceName;
        CachedSettingValue<QString> m_networkInterfaceAddress;
        CachedSettingValue<int> m_encryption;
        CachedSettingValue<int> m_maxActiveCheckingTorrents;
        CachedSettingValue<bool> m_isProxyPeerConnectionsEnabled;
        CachedSettingValue<ChokingAlgorithm> m_chokingAlgorithm;
        CachedSettingValue<SeedChokingAlgorithm> m_seedChokingAlgorithm;
        CachedSettingValue<QStringList> m_storedTags;
        CachedSettingValue<int> m_maxRatioAction;
        CachedSettingValue<Path> m_savePath;
        CachedSettingValue<Path> m_downloadPath;
        CachedSettingValue<bool> m_isDownloadPathEnabled;
        CachedSettingValue<bool> m_isSubcategoriesEnabled;
        CachedSettingValue<bool> m_useCategoryPathsInManualMode;
        CachedSettingValue<bool> m_isAutoTMMDisabledByDefault;
        CachedSettingValue<bool> m_isDisableAutoTMMWhenCategoryChanged;
        CachedSettingValue<bool> m_isDisableAutoTMMWhenDefaultSavePathChanged;
        CachedSettingValue<bool> m_isDisableAutoTMMWhenCategorySavePathChanged;
        CachedSettingValue<bool> m_isTrackerEnabled;
        CachedSettingValue<int> m_peerTurnover;
        CachedSettingValue<int> m_peerTurnoverCutoff;
        CachedSettingValue<int> m_peerTurnoverInterval;
        CachedSettingValue<int> m_requestQueueSize;
        CachedSettingValue<bool> m_isExcludedFileNamesEnabled;
        CachedSettingValue<QStringList> m_excludedFileNames;
        CachedSettingValue<QStringList> m_bannedIPs;
        CachedSettingValue<ResumeDataStorageType> m_resumeDataStorageType;

        bool m_isRestored = false;

        // Order is important. This needs to be declared after its CachedSettingsValue
        // counterpart, because it uses it for initialization in the constructor
        // initialization list.
        const bool m_wasPexEnabled = m_isPeXEnabled;

        int m_numResumeData = 0;
        int m_extraLimit = 0;
        QVector<TrackerEntry> m_additionalTrackerList;
        QVector<QRegularExpression> m_excludedFileNamesRegExpList;

        bool m_refreshEnqueued = false;
        QTimer *m_seedingLimitTimer = nullptr;
        QTimer *m_resumeDataTimer = nullptr;
        Statistics *m_statistics = nullptr;
        // IP filtering
        QPointer<FilterParserThread> m_filterParser;
        QPointer<BandwidthScheduler> m_bwScheduler;
        // Tracker
        QPointer<Tracker> m_tracker;

        QThread *m_ioThread = nullptr;
        ResumeDataStorage *m_resumeDataStorage = nullptr;
        FileSearcher *m_fileSearcher = nullptr;

        QSet<TorrentID> m_downloadedMetadata;

        QHash<TorrentID, TorrentImpl *> m_torrents;
        QHash<TorrentID, LoadTorrentParams> m_loadingTorrents;
        QHash<QString, AddTorrentParams> m_downloadedTorrents;
        QHash<TorrentID, RemovingTorrentData> m_removingTorrents;
        QSet<TorrentID> m_needSaveResumeDataTorrents;
        QMap<QString, CategoryOptions> m_categories;
        QSet<QString> m_tags;

        QHash<Torrent *, QSet<QString>> m_updatedTrackerEntries;

        // I/O errored torrents
        QSet<TorrentID> m_recentErroredTorrents;
        QTimer *m_recentErroredTorrentsTimer = nullptr;

        SessionMetricIndices m_metricIndices;
        lt::time_point m_statsLastTimestamp = lt::clock_type::now();

        SessionStatus m_status;
        CacheStatus m_cacheStatus;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QNetworkConfigurationManager *m_networkManager = nullptr;
#endif

        QList<MoveStorageJob> m_moveStorageQueue;

        QString m_lastExternalIP;

        bool m_needUpgradeDownloadPath = false;

        static Session *m_instance;
    };
}
