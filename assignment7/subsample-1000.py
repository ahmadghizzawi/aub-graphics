import codecs, json
import glob
import numpy as np
from json import dump
import collections

class JSONEncoder(json.JSONEncoder):
    def default(self, obj):
        if hasattr(obj, '__json__'):
            return obj.__json__()
        elif isinstance(obj, collections.Iterable):
            return list(obj)
        elif isinstance(obj, datetime):
            return obj.isoformat()
        elif hasattr(obj, '__getitem__') and hasattr(obj, 'keys'):
            return dict(obj)
        elif hasattr(obj, '__dict__'):
            return {member: getattr(obj, member)
                    for member in dir(obj)
                    if not member.startswith('_') and
                    not hasattr(getattr(obj, member), '__call__')}

        return json.JSONEncoder.default(self, obj)


def writeToFile(batch, batch_number):
    with open(str(batch_number) + "_batch.json", "w") as output:
        dump(batch, output, cls=JSONEncoder)

tweets = np.array([])

for jsonFile in glob.glob('./*.csv.json'):
    print(jsonFile)
    with codecs.open(jsonFile, 'r', 'utf-8') as f:
        batch = np.array(json.load(f, encoding='utf-8'))
        tweets = np.append(tweets, batch)
        print("Dataset size: " + str(tweets.shape))

# pick 1000 tweets uniformly from the Dataset
sample = np.random.choice(tweets, 2000, replace=False)

count = 1
batch_number = 0
batch = np.array([])
for tweet in sample:
    tweet['ds_id'] =  count
    batch = np.append(batch, [tweet])
    if count % 100 == 0 and count:
        batch_number+=1
        print(batch.shape)
        writeToFile(batch, batch_number)
        batch = np.array([])
    count += 1
print(count)
