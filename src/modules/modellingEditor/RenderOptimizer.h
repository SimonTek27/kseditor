#pragma once

#include <QVector>
#include <QMap>
#include <QVector3D>
#include <QString>
#include <memory>

namespace ks {
class SceneObject;

/**
 * @brief Configurazione per l'ottimizzazione del rendering
 */
struct RenderOptimizationConfig {
	bool enableBatching = true;
	bool enableFrustumCulling = true;
	bool enableLOD = true;
	bool enableGeometryCaching = true;
	bool enableVSync = true;
	int targetFrameRate = 60;

	// Distanze LOD (default distanze)
	float lodDistance0 = 10.0f;  // Massima dettagli
	float lodDistance1 = 25.0f;  // Medium dettagli
	float lodDistance2 = 50.0f;  // Basso dettagli
	float lodDistance3 = 100.0f; // Wireframe/Hidden

	// Frustum culling
	bool enableAggressiveCulling = false; // More aggressive culling for performance
};

/**
 * @brief Batch per mesh rendering
 * Raggruppa oggetti con lo stesso materiale per ridurre draw calls
 */
struct MeshBatch {
	QString materialId;
	QVector<int> objectIds;
	int vertexCount = 0;
	int triangleCount = 0;
	bool isDirty = true;
};

/**
 * @brief Manager per l'ottimizzazione rendering
 */
class RenderOptimizer {
public:
	explicit RenderOptimizer();
	~RenderOptimizer();

	/**
	 * @brief Configura le opzioni di ottimizzazione
	 */
	void setConfig(const RenderOptimizationConfig& config);

	/**
	 * @brief Ottieni configurazione attuale
	 */
	const RenderOptimizationConfig& getConfig() const { return m_config; }

	/**
	 * @brief Aggiorna lo stato di renderizzazione
	 * Eseguire una volta per frame prima di renderizzare
	 */
	void updateFrame(
		const QVector3D& cameraPosition,
		const QVector3D& cameraDirection,
		float cameraFOV,
		float aspectRatio
	);

	/**
	 * @brief Controlla se un oggetto deve essere renderizzato
	 */
	bool shouldRenderObject(
		int objectId,
		const QVector3D& objectPosition,
		float objectRadius
	) const;

	/**
	 * @brief Calcola il LOD level basato sulla distanza camera
	 */
	int calculateLODLevel(
		const QVector3D& objectPosition,
		const QVector3D& cameraPosition
	) const;

	/**
	 * @brief Ottimizza batches di mesh
	 * Raggruppa oggetti per materiale per ridurre draw calls
	 */
	QVector<MeshBatch> optimizeBatches(const QVector<SceneObject*>& objects);

	/**
	 * @brief Invalida la cache (dopo modifica mesh)
	 */
	void invalidateCache(int objectId);

	/**
	 * @brief Invalida tutte le cache
	 */
	void invalidateAllCaches();

	/**
	 * @brief Ottieni statistiche rendering
	 */
	struct RenderStats {
		int visibleObjectCount = 0;
		int culledObjectCount = 0;
		int batchCount = 0;
		int drawCallCount = 0;
		float estimatedFPS = 60.0f;
	};

	RenderStats getStats() const { return m_stats; }

private:
	RenderOptimizationConfig m_config;
	RenderStats m_stats;
	QMap<int, QVector<MeshBatch>> m_cache;

	// Frustum culling
	struct Frustum {
		QVector3D position;
		QVector3D direction;
		float fov = 45.0f;
		float aspect = 16.0f / 9.0f;
		float nearPlane = 0.1f;
		float farPlane = 1000.0f;
	};
	Frustum m_frustum;

	bool isSphereInFrustum(const QVector3D& center, float radius) const;
};

/**
 * @brief Cache per geometria mesh
 * Evita rebuild della geometria quando non modificata
 */
class MeshGeometryCache {
public:
	struct CacheEntry {
		int vertexCount = 0;
		int triangleCount = 0;
		QVector<float> vertexData;
		QVector<uint32_t> indexData;
		bool isValid = false;
		bool isDirty = false;
	};

	void setCacheEntry(int objectId, const CacheEntry& entry);
	bool getCacheEntry(int objectId, CacheEntry& outEntry) const;
	bool isCacheValid(int objectId) const;
	void invalidateEntry(int objectId);
	void invalidateAll();

	int getCacheMemoryUsage() const;
	void limitCacheSize(int maxMemoryMB);

private:
	QMap<int, CacheEntry> m_cache;
	int m_totalMemoryUsage = 0;
};

} // namespace ks
