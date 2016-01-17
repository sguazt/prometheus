import com.tdunning.math.stats.*;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.Reader;


public class TDigestClient
{
	private static final String DEFAULT_TDIGEST = "default";
	private static final int DEFAULT_COMPRESSION = 100;

	public static void main(String[] args) throws Throwable
	{
		if (args.length < 2)
		{
			System.err.println("ERROR: Wrong number of arguments.");
			System.err.println("Usage: java " + TDigestClient.class.getName() + " <probability> <data file> [<digest> <compression>]");
			System.exit(1);
		}

		double prob = 0;
		String datafile = null;
		String whatDigest = DEFAULT_TDIGEST;
		int compression = DEFAULT_COMPRESSION;

		try
		{
			prob = Double.parseDouble(args[0]);

			if (prob < 0)
			{
				throw new RuntimeException("Probability must be a nonnegative number");
			}
		}
		catch (Throwable t)
		{
			System.err.println("ERROR: Bad probability '" + t + "'");
			System.exit(1);
		}
		datafile = args[1];
		if (args.length > 2)
		{
			whatDigest = args[2];
		}
		if (args.length > 3)
		{
			try
			{
				compression = Integer.parseInt(args[3]);

				if (compression <= 0)
				{
					throw new RuntimeException("Probability must be a positive number");
				}
			}
			catch (Throwable t)
			{
				System.err.println("ERROR: Bad compression '" + t + "'");
				System.exit(1);
			}
		}

		TDigest tdigest = null;

		if ("array".equalsIgnoreCase(whatDigest))
		{
			tdigest = TDigest.createArrayDigest(compression);
		}
		else if ("avl".equalsIgnoreCase(whatDigest))
		{
			tdigest = TDigest.createAvlTreeDigest(compression);
		}
		else if ("default".equalsIgnoreCase(whatDigest))
		{
			tdigest = TDigest.createDigest(compression);
		}
		else if ("tree".equalsIgnoreCase(whatDigest))
		{
			tdigest = TDigest.createTreeDigest(compression);
		}
		else
		{
			System.err.println("ERROR: Unknown digest '" + whatDigest + "'");
			System.exit(1);
		}

		BufferedReader rd = null;

		try
		{
			rd = new BufferedReader(new FileReader(datafile));

			String line = null;

			while ((line = rd.readLine()) != null)
			{
				double val = Double.parseDouble(line);

				//tdigest.add(val*1e-9);
				tdigest.add(val);
				System.out.println(tdigest.quantile(prob));
			}
		}
		catch (Throwable t)
		{
			System.err.println("ERROR: Caught exception: " + t);
		}

		System.err.println("Final " + (prob*100.0) + "-th percentile: " + tdigest.quantile(prob));
	}
}
