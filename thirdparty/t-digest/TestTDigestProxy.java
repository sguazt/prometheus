import tdigestx.*;

public class TestTDigestProxy
{
	public static void main(String[] args)
	{
		TDigestProxy proxy = new TDigestProxy();

		proxy.init();

		proxy.add(1.0);
		proxy.add(1.5);
		proxy.add(2.0);

		System.out.println("Quantile: " + proxy.quantile(.5));
	}
}
