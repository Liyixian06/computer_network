from flask import Flask, render_template
app = Flask(__name__)

@app.route('/test', methods=['GET'])
def test():
    return render_template('test.html')

if __name__=="__main__":
    app.run(debug=True, port=5000)