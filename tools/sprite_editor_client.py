"""CLI for the explicitly enabled local Sprite Editor connection."""
import argparse
import json
from pathlib import Path
from urllib.request import Request, urlopen
from urllib.parse import urlparse


def request(connection, route, body=None):
    config=json.loads(Path(connection).read_text(encoding='utf-8'))
    url=urlparse(config['url'])
    if url.scheme!='http' or url.hostname!='127.0.0.1' or url.path or url.username or url.password:
        raise ValueError('Expected a local Editor endpoint')
    data=None if body is None else json.dumps(body).encode()
    req=Request(config['url']+route,data=data,headers={'Authorization':'Bearer '+config['token'],'Content-Type':'application/json'})
    with urlopen(req,timeout=8) as response:return response.read()


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--connection',required=True)
    parser.add_argument('command',choices=['state','collections','open','source','command','history'])
    parser.add_argument('--collection-id');parser.add_argument('--session');parser.add_argument('--index',type=int);parser.add_argument('--out')
    parser.add_argument('--route', choices=['/material-edit','/material-undo','/save-project','/load-project','/pbr-set'])
    parser.add_argument('--body-file', help='UTF-8 JSON command body; session and revision are required')
    args=parser.parse_args()
    if args.command=='command':
        if not args.route or not args.body_file:parser.error('command requires --route and --body-file')
        result=request(args.connection,args.route,json.loads(Path(args.body_file).read_text(encoding='utf-8')))
    elif args.command=='history':
        if not args.session:parser.error('history requires --session')
        result=request(args.connection,'/history/'+args.session)
    elif args.command=='open':
        if not args.collection_id or not args.session:parser.error('open requires --collection-id and --session')
        result=request(args.connection,'/open-collection',dict(collection_id=args.collection_id,project_session=args.session))
    elif args.command=='source':
        if not args.session or args.index is None or not args.out:parser.error('source requires --session, --index and --out')
        result=request(args.connection,f'/source/{args.session}/{args.index}/image.png')
        with open(args.out,'xb') as output:output.write(result)
        print('Saved '+args.out);raise SystemExit(0)
    else:result=request(args.connection,'/'+args.command)
    print(result.decode('utf-8'))
