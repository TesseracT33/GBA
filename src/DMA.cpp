module DMA;

namespace DMA
{
	void Initialize()
	{
		std::memset(&dma_0, 0, sizeof(dma_0));
		std::memset(&dma_1, 0, sizeof(dma_1));
		std::memset(&dma_2, 0, sizeof(dma_2));
		std::memset(&dma_3, 0, sizeof(dma_3));
	}
}